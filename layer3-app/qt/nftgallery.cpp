#include "nftgallery.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QCoreApplication>
#include <QDir>
#include <QGridLayout>
#include <QGroupBox>

#include "nft_ui_helpers.h"

NftGallery::NftGallery(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* v = new QVBoxLayout(this);
    tree = new QTreeWidget(this);
    tree->setRootIsDecorated(false);
    const auto headers = nft::ui::HeaderLabels();
    QStringList qheaders;
    for (const auto& h : headers) qheaders << QString::fromStdString(h);
    tree->setHeaderLabels(qheaders);
    tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    transfer_btn = new QPushButton("Transfer", this);
    mint_btn = new QPushButton("Mint (if allowed)", this);

    v->addWidget(tree);

    QGroupBox* detail = new QGroupBox("NFT detail", this);
    QGridLayout* detail_layout = new QGridLayout(detail);
    preview = new QLabel(detail);
    preview->setMinimumSize(96, 96);
    preview->setAlignment(Qt::AlignCenter);
    title_label = new QLabel(detail);
    category_label = new QLabel(detail);
    creator_label = new QLabel(detail);
    royalty_label = new QLabel(detail);
    owner_label = new QLabel(detail);
    history_label = new QLabel(detail);
    history_label->setWordWrap(true);
    listing_label = new QLabel(detail);
    bids_label = new QLabel(detail);
    last_sale_label = new QLabel(detail);

    detail_layout->addWidget(preview, 0, 0, 3, 1);
    detail_layout->addWidget(new QLabel("Title", detail), 0, 1);
    detail_layout->addWidget(title_label, 0, 2);
    detail_layout->addWidget(new QLabel("Canon category", detail), 1, 1);
    detail_layout->addWidget(category_label, 1, 2);
    detail_layout->addWidget(new QLabel("Creator", detail), 2, 1);
    detail_layout->addWidget(creator_label, 2, 2);
    detail_layout->addWidget(new QLabel("Royalty", detail), 3, 1);
    detail_layout->addWidget(royalty_label, 3, 2);
    detail_layout->addWidget(new QLabel("Owner", detail), 4, 1);
    detail_layout->addWidget(owner_label, 4, 2);
    detail_layout->addWidget(new QLabel("Ownership history", detail), 5, 1);
    detail_layout->addWidget(history_label, 5, 2);
    detail_layout->addWidget(new QLabel("Marketplace", detail), 6, 1);
    detail_layout->addWidget(listing_label, 6, 2);
    detail_layout->addWidget(new QLabel("Bids", detail), 7, 1);
    detail_layout->addWidget(bids_label, 7, 2);
    detail_layout->addWidget(new QLabel("Last sale", detail), 8, 1);
    detail_layout->addWidget(last_sale_label, 8, 2);
    v->addWidget(detail);

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addWidget(transfer_btn);
    buttons->addWidget(mint_btn);
    buttons->addStretch(1);
    v->addLayout(buttons);

    connect(tree, &QTreeWidget::itemSelectionChanged, this, &NftGallery::update_selection);
    connect(transfer_btn, &QPushButton::clicked, this, &NftGallery::trigger_transfer);
    connect(mint_btn, &QPushButton::clicked, this, &NftGallery::mint_requested);
}

void NftGallery::set_items(const QList<NftItem>& items)
{
    items_ = items;
    tree->clear();
    for (int i = 0; i < items_.size(); ++i) {
        const auto& item = items_.at(i);
        auto* row = new QTreeWidgetItem(
            {item.title, item.canon_category, item.creator,
             QString::number(item.royalty_bps), item.owner, item.bids, item.last_sale});
        row->setData(0, Qt::UserRole, i);
        row->setIcon(0, QIcon(icon_for_category(item.canon_category)));
        tree->addTopLevelItem(row);
    }
    tree->resizeColumnToContents(0);
    if (!items_.isEmpty()) {
        tree->setCurrentItem(tree->topLevelItem(0));
        update_selection();
    } else {
        clear();
    }
}

void NftGallery::trigger_transfer()
{
    auto* current = tree->currentItem();
    if (!current) {
        QMessageBox::warning(this, "NFT", "Select an NFT to transfer.");
        return;
    }
    bool ok = false;
    const int idx = current->data(0, Qt::UserRole).toInt(&ok);
    const int count = items_.size();
    if (!ok || idx < 0 || idx >= count) return;
    const QString token_id = items_.at(idx).token_id;
    QString dest = QInputDialog::getText(this, "Transfer NFT", "Recipient address", QLineEdit::Normal, "0x...", &ok);
    if (ok && !dest.isEmpty()) {
        emit transfer_requested(token_id, dest);
    }
}

void NftGallery::update_selection()
{
    auto* current = tree->currentItem();
    if (!current) {
        clear();
        return;
    }
    bool ok = false;
    const int idx = current->data(0, Qt::UserRole).toInt(&ok);
    const int count = items_.size();
    if (!ok || idx < 0 || idx >= count) return;
    const auto& item = items_.at(idx);
    const QString icon_path = icon_for_category(item.canon_category);
    QPixmap pm(icon_path);
    if (pm.isNull() && !item.image_url.isEmpty()) {
        pm = QPixmap(item.image_url);
    }
    if (!pm.isNull()) {
        preview->setPixmap(pm.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        preview->clear();
    }
    title_label->setText(QString("%1 (ID %2)").arg(item.title, item.token_id));
    category_label->setText(item.canon_category);
    creator_label->setText(item.creator);
    royalty_label->setText(QString::number(item.royalty_bps) + " bps");
    owner_label->setText(item.owner);
    history_label->setText(item.ownership_history.isEmpty() ? "No history yet" : item.ownership_history);
    if (!item.listing_asset.isEmpty() && item.listing_price > 0) {
        listing_label->setText(
            QString("Listed: %1 %2").arg(item.listing_price, 0, 'f', 2).arg(item.listing_asset));
    } else {
        listing_label->setText("Not listed");
    }
    bids_label->setText(item.bids.isEmpty() ? "No bids" : item.bids);
    last_sale_label->setText(item.last_sale.isEmpty() ? "No sale recorded" : item.last_sale);
}

QString NftGallery::assets_root() const
{
    static const QString kRepoAssets = "assets";
    static const QString kInstalledAssets = "../assets";
    static const QString kLegacyAssets = "layer3-app/assets";
    const QStringList candidates = {
        QDir::current().absoluteFilePath(kRepoAssets),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(kInstalledAssets),
        QDir::current().absoluteFilePath(kLegacyAssets)};
    for (const auto& c : candidates) {
        if (QDir(c).exists()) return c;
    }
    return QDir::current().absoluteFilePath(kRepoAssets);
}

QString NftGallery::icon_for_category(const QString& canon_category) const
{
    const auto path =
        nft::ui::ResolveIconPath(assets_root().toStdString(), canon_category.toStdString());
    return QString::fromStdString(path);
}

void NftGallery::clear()
{
    preview->clear();
    title_label->clear();
    category_label->clear();
    creator_label->clear();
    royalty_label->clear();
    owner_label->clear();
    history_label->clear();
    listing_label->setText("Not listed");
    bids_label->clear();
    last_sale_label->clear();
}
