#ifndef SUREDO_H
#define SUREDO_H

#include <QDialog>

namespace Ui {
class SureDo;
}

class SureDo : public QDialog
{
    Q_OBJECT

public:
    explicit SureDo(QWidget *parent = nullptr);
    ~SureDo() override;

    void set_text(const QString& text);

    void close_line() const;

    void clear_line();

    [[nodiscard]] QString get_line() const;

private slots:
    void on_buttonBox_rejected();

private:
    Ui::SureDo *ui;
};

#endif // SUREDO_H
