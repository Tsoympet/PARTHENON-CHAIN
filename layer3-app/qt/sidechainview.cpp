#include "sidechainview.h"

#include "contractcaller.h"
#include "nftgallery.h"
#include "dappbrowser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QUrl>

SidechainView::SidechainView(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* v = new QVBoxLayout(this);

    v->addWidget(build_balances_widget());
    v->addWidget(build_bridge_widget());
    v->addWidget(build_contracts_widget());
    v->addWidget(build_nft_widget());
    v->addWidget(build_dapps_widget(), 1);

    seed_mock_data();
}

QWidget* SidechainView::build_balances_widget()
{
    QGroupBox* box = new QGroupBox("Balances", this);
    QFormLayout* f = new QFormLayout(box);
    tln_balance_label = new QLabel("0 TLN", box);
    tln_balance_label->setToolTip("TLN (asset 0) is the base monetary unit; NFTs are isolated from it.");
    drm_balance_label = new QLabel("0 DRM", box);
    drm_balance_label->setToolTip("DRM (asset 1) funds WASM smart contracts and marketplace pricing.");
    obl_balance_label = new QLabel("0 OBL", box);
    obl_balance_label->setToolTip("OBL (asset 2) fuels dApps and marketplace bids.");
    f->addRow("TLN (base)", tln_balance_label);
    f->addRow("DRM (contracts/market)", drm_balance_label);
    f->addRow("OBL (dApps/market)", obl_balance_label);
    return box;
}

QWidget* SidechainView::build_bridge_widget()
{
    QGroupBox* box = new QGroupBox("Anchors & Fees", this);
    QHBoxLayout* h = new QHBoxLayout(box);
    QPushButton* lock_btn = new QPushButton("Submit checkpoint (DRM)", box);
    QPushButton* burn_btn = new QPushButton("Fund OBL fees", box);
    lock_btn->setToolTip("DRM-backed WASM contracts anchor execution roots; checkpoints are mandatory.");
    burn_btn->setToolTip("OBL is the only fee token for dApps; fund the OBL domain before calling dApps.");
    connect(lock_btn, &QPushButton::clicked, this, &SidechainView::request_lock_to_sidechain);
    connect(burn_btn, &QPushButton::clicked, this, &SidechainView::request_burn_from_sidechain);
    h->addWidget(lock_btn);
    h->addWidget(burn_btn);
    h->addStretch(1);
    status_label = new QLabel("Sidechain idle", box);
    peer_label = new QLabel("Peers: 0", box);
    sync_bar = new QProgressBar(box);
    sync_bar->setRange(0, 100);
    sync_bar->setValue(0);
    QVBoxLayout* status_layout = new QVBoxLayout();
    status_layout->addWidget(status_label);
    status_layout->addWidget(peer_label);
    status_layout->addWidget(sync_bar);
    h->addLayout(status_layout);
    return box;
}

QWidget* SidechainView::build_contracts_widget()
{
    QGroupBox* box = new QGroupBox("Smart Contracts", this);
    QVBoxLayout* v = new QVBoxLayout(box);
    contract_caller = new ContractCaller(box);
    v->addWidget(contract_caller);
    connect(contract_caller, &ContractCaller::call_requested, this, &SidechainView::request_contract_call);
    return box;
}

QWidget* SidechainView::build_nft_widget()
{
    QGroupBox* box = new QGroupBox("NFTs", this);
    QVBoxLayout* v = new QVBoxLayout(box);
    nft_gallery = new NftGallery(box);
    v->addWidget(nft_gallery);
    connect(nft_gallery, &NftGallery::transfer_requested, this, &SidechainView::request_nft_transfer);
    connect(nft_gallery, &NftGallery::mint_requested, this, &SidechainView::request_nft_mint);
    return box;
}

QWidget* SidechainView::build_dapps_widget()
{
    QGroupBox* box = new QGroupBox("dApps", this);
    QVBoxLayout* v = new QVBoxLayout(box);
    dapp_browser = new DappBrowser(box);
    v->addWidget(dapp_browser);
    connect(dapp_browser, &DappBrowser::url_requested, this, &SidechainView::request_open_dapp);
    return box;
}

void SidechainView::seed_mock_data()
{
    set_balances(3.21, 123.45, 42.00);
    set_sidechain_status("Synced", 78, 5);
    QList<NftItem> items;
    items << NftItem{"1",
                     "Athena Tablet",
                     "mythology",
                     "curator",
                     250,
                     "assets/nft-icons/nft-mythology.svg",
                     "museum",
                     "curator → museum",
                     "DRM",
                     1250.50,
                     "Bids: 1100 DRM (collector), 1200 DRM (gallery)",
                     "Sold at 1250.50 DRM to museum"}
          << NftItem{"42",
                     "Marathon Hero",
                     "hero",
                     "historian",
                     100,
                     "assets/nft-icons/nft-hero.svg",
                     "archive",
                     "historian → archive",
                     "OBL",
                     210.00,
                     "Bids: 180.00 OBL (runner)",
                     "Sold at 210.00 OBL to archive"}
          << NftItem{"314",
                     "Delphi Omphalos",
                     "monument",
                     "foundation",
                     50,
                     "assets/nft-icons/nft-monument.svg",
                     "foundation",
                     "foundation",
                     "",
                     0.0,
                     "",
                     ""};
    if (nft_gallery) nft_gallery->set_items(items);

    if (contract_caller) {
        contract_caller->apply_abi(R"({"module":"greeting.wasm","exports":["init","greet","set_greeting"]})");
    }

    if (dapp_browser) {
        dapp_browser->set_default_url(QUrl("http://localhost:8080"));
        dapp_browser->set_popular({
            {"Block explorer", QUrl("http://localhost:8080/explorer")},
            {"DEX demo", QUrl("http://localhost:8080/dex")},
            {"NFT marketplace", QUrl("http://localhost:8080/market")},
        });
    }
}

void SidechainView::set_balances(double tln_balance, double drm_balance, double obl_balance)
{
    tln_balance_label->setText(QString::number(tln_balance, 'f', 4) + " TLN");
    drm_balance_label->setText(QString::number(drm_balance, 'f', 4) + " DRM");
    obl_balance_label->setText(QString::number(obl_balance, 'f', 4) + " OBL");
}

void SidechainView::set_sidechain_status(const QString& status_text, int sync_progress, int peer_count)
{
    status_label->setText(status_text);
    sync_bar->setValue(sync_progress);
    peer_label->setText(QString("Peers: %1").arg(peer_count));
}

void SidechainView::apply_abi_json(const QString& json_text)
{
    if (contract_caller) contract_caller->apply_abi(json_text);
}
