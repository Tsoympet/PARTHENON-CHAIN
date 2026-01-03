#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

struct NftItem {
    QString token_id;
    QString title;
    QString canon_category;
    QString creator;
    uint16_t royalty_bps{0};
    QString image_url;
    QString owner;
    QString ownership_history;
    QString listing_asset;   // DRM / OBL only
    double listing_price{0.0};
    QString bids;
    QString last_sale;
};

class NftGallery : public QWidget {
    Q_OBJECT
public:
    explicit NftGallery(QWidget* parent = nullptr);

    void set_items(const QList<NftItem>& items);
    void clear();

signals:
    void transfer_requested(const QString& token_id, const QString& to);
    void mint_requested();

private:
    void trigger_transfer();
    void update_selection();
    QString assets_root() const;
    QString icon_for_category(const QString& canon_category) const;

    QTreeWidget* tree{nullptr};
    QPushButton* transfer_btn{nullptr};
    QPushButton* mint_btn{nullptr};
    QLabel* preview{nullptr};
    QLabel* title_label{nullptr};
    QLabel* category_label{nullptr};
    QLabel* creator_label{nullptr};
    QLabel* royalty_label{nullptr};
    QLabel* owner_label{nullptr};
    QLabel* history_label{nullptr};
    QLabel* listing_label{nullptr};
    QLabel* bids_label{nullptr};
    QLabel* last_sale_label{nullptr};
    QList<NftItem> items_;
};
