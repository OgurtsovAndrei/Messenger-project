/**
 * TLS Session Manager
 * (C) 2011 Jack Lloyd
 *     2023 René Meusel - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#include <botan/tls_session_manager.h>

#include <botan/tls_callbacks.h>
#include <botan/tls_policy.h>
#include <botan/rng.h>

namespace Botan::TLS {

Session_Manager::Session_Manager(RandomNumberGenerator& rng)
   : m_rng(rng) {}

std::optional<Session_Handle> Session_Manager::establish(const Session& session,
            std::optional<Session_ID> id,
            bool tls12_no_ticket)
{
   // By default, the session manager does not emit session tickets anyway
   BOTAN_UNUSED(tls12_no_ticket);
   BOTAN_ASSERT(session.side() == Connection_Side::Server,
                "Client tried to establish a session");

   // TODO: C++20 allows CTAD for template aliases (read: lock_guard_type), so
   //       technically we should be able to omit the explicit mutex type.
   //       Unfortuately clang does not agree, yet.
   lock_guard_type<recursive_mutex_type> lk(mutex());

   Session_Handle handle(id.value_or(m_rng.random_vec<Session_ID>(32)));
   store(session, handle);
   return handle;
}

std::optional<Session> Session_Manager::retrieve(const Session_Handle& handle,
                                                 Callbacks& callbacks,
                                                 const Policy& policy)
   {
   lock_guard_type<recursive_mutex_type> lk(mutex());

   auto session = retrieve_one(handle);
   if(!session.has_value())
      return std::nullopt;

   // A value of '0' means: No policy restrictions.
   const std::chrono::seconds policy_lifetime =
      (policy.session_ticket_lifetime().count() > 0)
         ? policy.session_ticket_lifetime()
         : std::chrono::seconds::max();

   // RFC 5077 3.3 -- "Old Session Tickets"
   //    A server MAY treat a ticket as valid for a shorter or longer period of
   //    time than what is stated in the ticket_lifetime_hint.
   //
   // RFC 5246 F.1.4 -- TLS 1.2
   //    If either party suspects that the session may have been compromised, or
   //    that certificates may have expired or been revoked, it should force a
   //    full handshake.  An upper limit of 24 hours is suggested for session ID
   //    lifetimes.
   //
   // RFC 8446 4.6.1 -- TLS 1.3
   //    A server MAY treat a ticket as valid for a shorter period of time than
   //    what is stated in the ticket_lifetime.
   //
   // Note: This disregards what is stored in the session (e.g. "lifetime_hint")
   //       and only takes the local policy into account. The lifetime stored in
   //       the sessions was taken from the same policy anyways and changes by
   //       the application should have an immediate effect.
   const auto ticket_age =
      std::chrono::duration_cast<std::chrono::seconds>(
         callbacks.tls_current_timestamp() - session->start_time());
   const bool expired = ticket_age > policy_lifetime;

   if(expired)
      {
      remove(handle);
      return std::nullopt;
      }
   else
      {
      return session;
      }
   }

std::vector<Session_with_Handle> Session_Manager::find(const Server_Information& info,
                                                       Callbacks& callbacks,
                                                       const Policy& policy)
   {
   lock_guard_type<recursive_mutex_type> lk(mutex());

   auto sessions_and_handles = find_all(info);

   // A value of '0' means: No policy restrictions. Session ticket lifetimes as
   // communicated by the server apply regardless.
   const std::chrono::seconds policy_lifetime =
      (policy.session_ticket_lifetime().count() > 0)
         ? policy.session_ticket_lifetime()
         : std::chrono::seconds::max();

   if(!sessions_and_handles.empty())
      {
      const auto now = callbacks.tls_current_timestamp();

      // TODO: C++20, use std::ranges::remove_if() once XCode and Android NDK caught up.
      sessions_and_handles.erase(
         std::remove_if(sessions_and_handles.begin(), sessions_and_handles.end(), [&] (const auto& session)
            {
            const auto age =
               std::chrono::duration_cast<std::chrono::seconds>(now - session.session.start_time());

            // RFC 5077 3.3 -- "Old Session Tickets"
            //    The ticket_lifetime_hint field contains a hint from the
            //    server about how long the ticket should be stored. [...]
            //    A client SHOULD delete the ticket and associated state when
            //    the time expires. It MAY delete the ticket earlier based on
            //    local policy.
            //
            // RFC 5246 F.1.4 -- TLS 1.2
            //    If either party suspects that the session may have been
            //    compromised, or that certificates may have expired or been
            //    revoked, it should force a full handshake.  An upper limit of
            //    24 hours is suggested for session ID lifetimes.
            //
            // RFC 8446 4.2.11.1 -- TLS 1.3
            //    The client's view of the age of a ticket is the time since the
            //    receipt of the NewSessionTicket message.  Clients MUST NOT
            //    attempt to use tickets which have ages greater than the
            //    "ticket_lifetime" value which was provided with the ticket.
            //
            // RFC 8446 4.6.1 -- TLS 1.3
            //    Clients MUST NOT cache tickets for longer than 7 days,
            //    regardless of the ticket_lifetime, and MAY delete tickets
            //    earlier based on local policy.
            //
            // Note: TLS 1.3 tickets with a lifetime longer than 7 days are
            //       rejected during parsing with an "Illegal Parameter" alert.
            //       Other suggestions are left to the application via
            //       Policy::session_ticket_lifetime(). Session lifetimes as
            //       communicated by the server via the "lifetime_hint" are
            //       obeyed regardless of the policy setting.
            const auto session_lifetime_hint = session.session.lifetime_hint();
            const bool expired = age > std::min(policy_lifetime, session_lifetime_hint);

            if(expired)
               { remove(session.handle); }

            return expired;
            }), sessions_and_handles.end());
      }

   // std::vector::resize() cannot be used as the vector's members aren't
   // default constructible.
   const auto session_limit = policy.maximum_session_tickets_per_client_hello();
   while(session_limit > 0 && sessions_and_handles.size() > session_limit)
      { sessions_and_handles.pop_back(); }

   // RFC 8446 Appendix C.4
   //    Clients SHOULD NOT reuse a ticket for multiple connections. Reuse of
   //    a ticket allows passive observers to correlate different connections.
   //
   // When reuse of session tickets is not allowed, remove all tickets to be
   // returned from the implementation's internal storage.
   if(!policy.reuse_session_tickets())
      {
      for(const auto& [session, handle] : sessions_and_handles)
         {
         if(!session.version().is_pre_tls_13() || !handle.is_id())
            {
            remove(handle);
            }
         }
      }

   return sessions_and_handles;
   }

#if defined(BOTAN_HAS_TLS_13)

std::optional<std::pair<Session, uint16_t>>
      Session_Manager::choose_from_offered_tickets(const std::vector<Ticket>& tickets,
                                                   const std::string& hash_function,
                                                   Callbacks& callbacks,
                                                   const Policy& policy)
   {
   lock_guard_type<recursive_mutex_type> lk(mutex());

   for(uint16_t i = 0; const auto& ticket : tickets)
      {
      auto session = retrieve(ticket.identity(), callbacks, policy);
      if(session.has_value() && session->ciphersuite().prf_algo() == hash_function)
         {
         return std::pair{std::move(session.value()), i};
         }

      // RFC 8446 4.2.10
      //    For PSKs provisioned via NewSessionTicket, a server MUST validate
      //    that the ticket age for the selected PSK identity [...] is within a
      //    small tolerance of the time since the ticket was issued.  If it is
      //    not, the server SHOULD proceed with the handshake but reject 0-RTT,
      //    and SHOULD NOT take any other action that assumes that this
      //    ClientHello is fresh.
      //
      // TODO: The ticket-age is currently not checked (as 0-RTT is not
      //       implemented) and we simply take the SHOULD at face value.
      //       Instead we could add a policy check letting the user decide.

      ++i;
      }

   return std::nullopt;
   }

#endif

}
