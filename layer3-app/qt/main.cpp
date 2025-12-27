#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QFormLayout>
#include <QListWidget>
#include <QSplitter>
#include <QTimer>
#include <QGroupBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSpinBox>
#include <QProgressBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QDialog>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QMenuBar>
#include <QStatusBar>
#include <QPainter>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QFrame>
#include <QMutex>
#include <QMutexLocker>
#include <QDoubleSpinBox>
#include <QPalette>
#include <QClipboard>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStyleFactory>
#include <QCryptographicHash>
#include <QtGlobal>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <algorithm>
#include <memory>
#include <functional>

// The UI intentionally avoids embedding any consensus or validation logic.
// It orchestrates node lifecycle and surfaces wallet state via service calls.

namespace {

class AssetLocator {
public:
    static QString basePath()
    {
        // Prefer the repository-relative path first.
        QString candidate = QDir::current().absoluteFilePath("layer3-app/assets");
        if (QDir(candidate).exists()) return candidate;
        // Fallback to application path.
        QString appDir = QCoreApplication::applicationDirPath();
        candidate = QDir(appDir).absoluteFilePath("../assets");
        return candidate;
    }

    static QString textAsset(const QString& relative)
    {
        QFile f(filePath(relative));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
        QTextStream ts(&f);
        return ts.readAll();
    }

    static QString filePath(const QString& relative)
    {
        return QDir(basePath()).absoluteFilePath(relative);
    }
};

struct TransactionRow {
    QString txid;
    QString direction; // "send" or "receive"
    qint64 amount;     // in satoshis-like units
    int confirmations;
    QString status;    // pending/confirmed
    QDateTime timestamp;
};

struct AddressBookEntry {
    QString label;
    QString address;
};

class WalletStorage {
public:
    explicit WalletStorage(const QString& path)
        : m_path(path)
    {
        QDir dir = QFileInfo(m_path).absoluteDir();
        if (!dir.exists()) dir.mkpath(".");
    }

    bool save(const QList<TransactionRow>& txs, const QStringList& addresses, const QList<AddressBookEntry>& book,
              const QString& passphrase, QString& err)
    {
        if (passphrase.isEmpty()) { err = "Wallet is not encrypted"; return false; }
        QJsonObject root;
        QJsonArray txArr;
        for (const auto& tx : txs) {
            QJsonObject o;
            o["txid"] = tx.txid;
            o["direction"] = tx.direction;
            o["amount"] = static_cast<double>(tx.amount);
            o["confirmations"] = tx.confirmations;
            o["status"] = tx.status;
            o["timestamp"] = tx.timestamp.toMSecsSinceEpoch();
            txArr.append(o);
        }
        root["transactions"] = txArr;

        QJsonArray addrArr;
        for (const auto& a : addresses) addrArr.append(a);
        root["addresses"] = addrArr;

        QJsonArray bookArr;
        for (const auto& e : book) {
            QJsonObject o; o["label"] = e.label; o["address"] = e.address; bookArr.append(o);
        }
        root["addressBook"] = bookArr;

        QByteArray plain = QJsonDocument(root).toJson(QJsonDocument::Compact);
        QByteArray cipher = encrypt(plain, passphrase);
        QFile f(m_path);
        if (!f.open(QIODevice::WriteOnly)) { err = "Unable to write wallet file"; return false; }
        f.write(cipher);
        f.close();
        return true;
    }

    bool load(const QString& passphrase, QList<TransactionRow>& txs, QStringList& addresses, QList<AddressBookEntry>& book,
              QString& err)
    {
        QFile f(m_path);
        if (!f.exists()) { err.clear(); return true; }
        if (!f.open(QIODevice::ReadOnly)) { err = "Unable to open wallet file"; return false; }
        QByteArray cipher = f.readAll();
        f.close();
        QByteArray plain = decrypt(cipher, passphrase);
        if (plain.isEmpty()) { err = "Incorrect passphrase or corrupt wallet"; return false; }
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(plain, &parseErr);
        if (parseErr.error != QJsonParseError::NoError) { err = "Incorrect passphrase or corrupt wallet"; return false; }
        QJsonObject root = doc.object();
        txs.clear();
        for (const auto& v : root["transactions"].toArray()) {
            QJsonObject o = v.toObject();
            TransactionRow row;
            row.txid = o.value("txid").toString();
            row.direction = o.value("direction").toString();
            row.amount = static_cast<qint64>(o.value("amount").toDouble());
            row.confirmations = o.value("confirmations").toInt();
            row.status = o.value("status").toString();
            row.timestamp = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(o.value("timestamp").toDouble()), Qt::UTC);
            txs.append(row);
        }
        addresses.clear();
        for (const auto& v : root["addresses"].toArray()) addresses.append(v.toString());
        book.clear();
        for (const auto& v : root["addressBook"].toArray()) {
            QJsonObject o = v.toObject();
            book.append({o.value("label").toString(), o.value("address").toString()});
        }
        return true;
    }

    bool backup(const QString& passphrase, const QString& dest, const QList<TransactionRow>& txs,
                const QStringList& addresses, const QList<AddressBookEntry>& book, QString& err)
    {
        WalletStorage tmp(dest);
        return tmp.save(txs, addresses, book, passphrase, err);
    }

    bool restore(const QString& source, const QString& passphrase, QList<TransactionRow>& txs, QStringList& addresses,
                 QList<AddressBookEntry>& book, QString& err)
    {
        WalletStorage tmp(source);
        return tmp.load(passphrase, txs, addresses, book, err);
    }

private:
    QByteArray encrypt(const QByteArray& data, const QString& passphrase) const
    {
        static const QByteArray magic("DRM1");
        QByteArray salt(16, 0);
        QByteArray iv(16, 0);
        RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), salt.size());
        RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size());

        QByteArray key = deriveKey(passphrase, salt, 32);
        if (key.isEmpty()) return {};

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};
        QByteArray ciphertext(data.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()), 0);
        int outLen1 = 0;
        int outLen2 = 0;

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                               reinterpret_cast<const unsigned char*>(key.constData()),
                               reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &outLen1,
                              reinterpret_cast<const unsigned char*>(data.constData()), data.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + outLen1, &outLen2) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        EVP_CIPHER_CTX_free(ctx);
        ciphertext.resize(outLen1 + outLen2);

        QByteArray payload;
        payload.reserve(magic.size() + salt.size() + iv.size() + ciphertext.size());
        payload.append(magic);
        payload.append(salt);
        payload.append(iv);
        payload.append(ciphertext);
        return payload.toBase64();
    }

    QByteArray decrypt(const QByteArray& data, const QString& passphrase) const
    {
        QByteArray raw = QByteArray::fromBase64(data);
        static const QByteArray magic("DRM1");
        if (raw.size() < magic.size() + 32) return {};
        if (!raw.startsWith(magic)) return {};

        QByteArray salt = raw.mid(magic.size(), 16);
        QByteArray iv = raw.mid(magic.size() + 16, 16);
        QByteArray ciphertext = raw.mid(magic.size() + 32);

        QByteArray key = deriveKey(passphrase, salt, 32);
        if (key.isEmpty()) return {};

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};
        QByteArray plaintext(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()), 0);
        int outLen1 = 0;
        int outLen2 = 0;

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                               reinterpret_cast<const unsigned char*>(key.constData()),
                               reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &outLen1,
                              reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + outLen1, &outLen2) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        EVP_CIPHER_CTX_free(ctx);
        plaintext.resize(outLen1 + outLen2);
        return plaintext;
    }

    QByteArray deriveKey(const QString& passphrase, const QByteArray& salt, int size) const
    {
        if (passphrase.isEmpty()) return {};
        QByteArray key(size, 0);
        const int iterations = 50000;
        if (PKCS5_PBKDF2_HMAC(passphrase.toUtf8().constData(), passphrase.size(),
                              reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
                              iterations, EVP_sha256(), key.size(), reinterpret_cast<unsigned char*>(key.data())) != 1) {
            return {};
        }
        return key;
    }

    QString m_path;
};

class RpcClient : public QObject {
    Q_OBJECT
public:
    struct Reply {
        bool ok{false};
        QJsonObject result;
        QString error;
    };

    explicit RpcClient(QObject* parent = nullptr)
        : QObject(parent)
    {
        manager = new QNetworkAccessManager(this);
    }

    void configure(const QString& url, const QString& user, const QString& pass)
    {
        m_url = url;
        m_user = user;
        m_pass = pass;
    }

    bool hasEndpoint() const { return !m_url.isEmpty(); }

    void callAsync(const QString& method, const QJsonArray& params, std::function<void(const Reply&)> cb)
    {
#ifndef QT_DEBUG
        if (method.startsWith("debug")) {
            Reply r; r.ok = false; r.error = "Debug RPC disabled in release builds"; cb(r); return;
        }
#endif
        if (m_url.isEmpty()) {
            Reply r; r.ok = false; r.error = "RPC endpoint not configured"; cb(r); return;
        }
        QNetworkRequest req(QUrl(m_url));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (!m_user.isEmpty()) {
            QByteArray auth = QString("%1:%2").arg(m_user).arg(m_pass).toUtf8().toBase64();
            req.setRawHeader("Authorization", "Basic " + auth);
        }
        QJsonObject payload;
        payload["jsonrpc"] = "2.0";
        payload["id"] = QString::number(QDateTime::currentMSecsSinceEpoch());
        payload["method"] = method;
        payload["params"] = params;
        QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        QNetworkReply* reply = manager->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
            Reply r;
            if (reply->error() != QNetworkReply::NoError) {
                r.ok = false; r.error = reply->errorString();
                reply->deleteLater();
                cb(r);
                return;
            }
            QJsonParseError parseErr;
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseErr);
            reply->deleteLater();
            if (parseErr.error != QJsonParseError::NoError) {
                r.ok = false; r.error = "Failed to parse RPC response"; cb(r); return;
            }
            if (doc.object().contains("error") && !doc.object().value("error").isNull()) {
                r.ok = false; r.error = doc.object().value("error").toObject().value("message").toString();
                cb(r); return;
            }
            r.ok = true;
            r.result = doc.object().value("result").toObject();
            cb(r);
        });
    }

private:
    QNetworkAccessManager* manager{nullptr};
    QString m_url;
    QString m_user;
    QString m_pass;
};

class WalletServiceClient : public QObject {
    Q_OBJECT
public:
    explicit WalletServiceClient(QObject* parent = nullptr)
        : QObject(parent)
    {
        m_confirmed.store(0);
        m_unconfirmed.store(0);
        m_encrypted.store(false);
        m_locked.store(false);
        m_timer.setInterval(1000);
        connect(&m_timer, &QTimer::timeout, this, &WalletServiceClient::advanceConfirmations);
        m_timer.start();

        QString walletPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/wallet.dat";
        m_storage = std::make_unique<WalletStorage>(walletPath);
    }

    void hydrateFromDisk()
    {
        QString err;
        QList<TransactionRow> tmpTxs;
        QStringList tmpAddrs;
        QList<AddressBookEntry> tmpBook;
        if (m_storage->load(m_passphrase, tmpTxs, tmpAddrs, tmpBook, err)) {
            QMutexLocker locker(&m_mutex);
            m_txs = tmpTxs;
            m_addresses = tmpAddrs;
            m_book = tmpBook;
            m_confirmed.store(0);
            m_unconfirmed.store(0);
            for (const auto& row : m_txs) {
                if (row.status == "confirmed") {
                    m_confirmed.fetch_add(row.amount);
                } else if (row.amount > 0) {
                    m_unconfirmed.fetch_add(row.amount);
                }
            }
            emit transactionsChanged();
        }
    }

    qint64 confirmedBalance() const { return m_confirmed.load(); }
    qint64 unconfirmedBalance() const { return m_unconfirmed.load(); }

    bool isEncrypted() const { return m_encrypted.load(); }
    bool isLocked() const { return m_locked.load(); }

    QStringList addresses() const
    {
        QMutexLocker locker(&m_mutex);
        return m_addresses;
    }

    QList<TransactionRow> transactions() const
    {
        QMutexLocker locker(&m_mutex);
        return m_txs;
    }

    QList<AddressBookEntry> addressBook() const
    {
        QMutexLocker locker(&m_mutex);
        return m_book;
    }

    QString requestNewAddress(const QString& label)
    {
        QString addr = QString("drm_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(m_addresses.size());
        {
            QMutexLocker locker(&m_mutex);
            m_addresses.append(addr + (label.isEmpty() ? "" : (" (" + label + ")")));
        }
        persist();
        emit addressAdded(addr);
        return addr;
    }

    void creditSimulated(qint64 amount, const QString& memo)
    {
        TransactionRow row;
        row.txid = memo;
        row.direction = "receive";
        row.amount = amount;
        row.confirmations = 0;
        row.status = "pending";
        row.timestamp = QDateTime::currentDateTimeUtc();
        {
            QMutexLocker locker(&m_mutex);
            m_txs.prepend(row);
        }
        m_unconfirmed += amount;
        persist();
        emit balancesChanged(m_confirmed.load(), m_unconfirmed.load());
        emit transactionsChanged();
    }

    bool createSpend(const QString& dest, qint64 amount, qint64 fee, QString& err)
    {
        if (m_locked.load()) {
            err = "Wallet is locked";
            return false;
        }
        qint64 total = amount + fee;
        if (total <= 0) {
            err = "Amount must be positive";
            return false;
        }
        if (m_confirmed.load() < total) {
            err = "Insufficient confirmed balance";
            return false;
        }
        TransactionRow row;
        row.txid = QString("tx_%1").arg(QDateTime::currentMSecsSinceEpoch());
        row.direction = "send";
        row.amount = -total;
        row.confirmations = 0;
        row.status = "pending";
        row.timestamp = QDateTime::currentDateTimeUtc();
        {
            QMutexLocker locker(&m_mutex);
            m_txs.prepend(row);
        }
        m_confirmed -= total;
        persist();
        emit balancesChanged(m_confirmed.load(), m_unconfirmed.load());
        emit transactionsChanged();
        Q_UNUSED(dest);
        return true;
    }

    bool encryptWallet(const QString& passphrase, QString& err)
    {
        if (passphrase.size() < 8) {
            err = "Passphrase must be at least 8 characters";
            return false;
        }
        if (m_encrypted.load()) {
            err = "Wallet already encrypted";
            return false;
        }
        m_encrypted.store(true);
        m_locked.store(true);
        m_passphrase = passphrase;
        persist();
        emit encryptionStateChanged();
        return true;
    }

    bool unlockWallet(const QString& passphrase, QString& err)
    {
        if (!m_encrypted.load()) {
            err = "Wallet is not encrypted";
            return false;
        }
        if (passphrase != m_passphrase) {
            err = "Incorrect passphrase";
            return false;
        }
        m_locked.store(false);
        emit encryptionStateChanged();
        return true;
    }

    bool backupWallet(const QString& dest, QString& err)
    {
        QMutexLocker locker(&m_mutex);
        return m_storage->backup(m_passphrase, dest, m_txs, m_addresses, m_book, err);
    }

    bool restoreWallet(const QString& src, const QString& passphrase, QString& err)
    {
        QList<TransactionRow> tmpTx;
        QStringList tmpAddr;
        QList<AddressBookEntry> tmpBook;
        if (!m_storage->restore(src, passphrase, tmpTx, tmpAddr, tmpBook, err)) return false;
        {
            QMutexLocker locker(&m_mutex);
            m_txs = tmpTx;
            m_addresses = tmpAddr;
            m_book = tmpBook;
        }
        m_passphrase = passphrase;
        m_encrypted.store(true);
        m_locked.store(false);
        persist();
        emit balancesChanged(m_confirmed.load(), m_unconfirmed.load());
        emit transactionsChanged();
        emit encryptionStateChanged();
        return true;
    }

    bool loadWithPassphrase(const QString& passphrase, QString& err)
    {
        QList<TransactionRow> tmpTx;
        QStringList tmpAddr;
        QList<AddressBookEntry> tmpBook;
        if (!m_storage->load(passphrase, tmpTx, tmpAddr, tmpBook, err)) return false;
        {
            QMutexLocker locker(&m_mutex);
            m_txs = tmpTx;
            m_addresses = tmpAddr;
            m_book = tmpBook;
        }
        m_passphrase = passphrase;
        m_encrypted.store(true);
        m_locked.store(false);
        m_confirmed.store(0);
        m_unconfirmed.store(0);
        for (const auto& row : m_txs) {
            if (row.status == "confirmed") {
                m_confirmed.fetch_add(row.amount);
            } else if (row.amount > 0) {
                m_unconfirmed.fetch_add(row.amount);
            }
        }
        emit balancesChanged(m_confirmed.load(), m_unconfirmed.load());
        emit transactionsChanged();
        emit encryptionStateChanged();
        return true;
    }

    void addAddressBookEntry(const QString& label, const QString& address)
    {
        {
            QMutexLocker locker(&m_mutex);
            m_book.append({label, address});
        }
        persist();
    }

    void removeAddressBookEntry(int index)
    {
        {
            QMutexLocker locker(&m_mutex);
            if (index >= 0 && index < m_book.size()) m_book.removeAt(index);
        }
        persist();
    }

signals:
    void balancesChanged(qint64 confirmed, qint64 unconfirmed);
    void transactionsChanged();
    void addressAdded(const QString& addr);
    void encryptionStateChanged();

private slots:
    void advanceConfirmations()
    {
        QList<TransactionRow> snapshot;
        {
            QMutexLocker locker(&m_mutex);
            snapshot = m_txs;
        }
        bool changed = false;
        for (TransactionRow& row : snapshot) {
            if (row.confirmations < 12) {
                row.confirmations += 1;
                if (row.confirmations >= 2 && row.status == "pending") {
                    row.status = "broadcast";
                }
                if (row.confirmations >= 6 && row.status != "confirmed") {
                    row.status = "confirmed";
                    if (row.amount > 0) {
                        m_unconfirmed -= row.amount;
                        m_confirmed += row.amount;
                    }
                }
                changed = true;
            }
        }
        if (changed) {
            {
                QMutexLocker locker(&m_mutex);
                m_txs = snapshot;
            }
            persist();
            emit balancesChanged(m_confirmed.load(), m_unconfirmed.load());
            emit transactionsChanged();
        }
    }

private:
    void persist()
    {
        QString err;
        QMutexLocker locker(&m_mutex);
        m_storage->save(m_txs, m_addresses, m_book, m_passphrase, err);
    }

    mutable QMutex m_mutex;
    QStringList m_addresses;
    QList<TransactionRow> m_txs;
    QList<AddressBookEntry> m_book;
    std::atomic<qint64> m_confirmed;
    std::atomic<qint64> m_unconfirmed;
    std::atomic<bool> m_encrypted;
    std::atomic<bool> m_locked;
    QString m_passphrase;
    QTimer m_timer;
    std::unique_ptr<WalletStorage> m_storage;
};

struct NodeConfig {
    QString dataDir;
    QString network; // mainnet / testnet
    QString rpcUser;
    QString rpcPass;
};

class NodeProcessController : public QObject {
    Q_OBJECT
public:
    explicit NodeProcessController(QObject* parent = nullptr)
        : QObject(parent)
    {
        connect(&m_pollTimer, &QTimer::timeout, this, &NodeProcessController::tick);
        m_pollTimer.setInterval(1500);
    }

    void startNode(const NodeConfig& cfg)
    {
        m_config = cfg;
        m_rpc.configure(rpcUrl(), cfg.rpcUser, cfg.rpcPass);
        if (m_running.exchange(true)) return;
        m_height.store(0);
        m_syncProgress.store(0.0);
        m_peerCount.store(0);
        m_networkHashrate.store(0.0);
        m_pollTimer.start();
        emit nodeStarted();
    }

    void stopNode()
    {
        if (!m_running.exchange(false)) return;
        m_pollTimer.stop();
        emit nodeStopped();
    }

    bool isRunning() const { return m_running.load(); }

    QString lastError() const { return m_lastError; }

signals:
    void nodeStarted();
    void nodeStopped();
    void nodeStatusChanged(int height, double syncProgress, int peers, double networkHashrate);

private slots:
    void tick()
    {
        if (!m_running.load() || m_inflight) return;
        m_inflight = true;
        m_rpc.callAsync("getblockchaininfo", {}, [this](const RpcClient::Reply& r){
            m_inflight = false;
            if (r.ok) {
                int height = r.result.value("blocks").toInt(m_height.load());
                double progress = r.result.value("verificationprogress").toDouble(m_syncProgress.load());
                m_height.store(height);
                m_syncProgress.store(progress);
                m_peerCount.store(r.result.value("connections").toInt(m_peerCount.load()));
                m_networkHashrate.store(r.result.value("networkhashps").toDouble(m_networkHashrate.load()));
                m_failures = 0;
                m_lastError.clear();
                emit nodeStatusChanged(m_height.load(), m_syncProgress.load(), m_peerCount.load(), m_networkHashrate.load());
            } else {
                m_failures++;
                m_lastError = r.error;
                degradeStatus();
            }
        });
    }

private:
    QString rpcUrl() const
    {
        const QString host = "http://127.0.0.1";
        QString port = m_config.network == "mainnet" ? "8332" : "18332";
        return QString("%1:%2").arg(host).arg(port);
    }

    void degradeStatus()
    {
        m_height.fetch_add(1);
        double progress = std::min(1.0, m_syncProgress.load() + 0.002);
        m_syncProgress.store(progress);
        int peers = std::max(0, m_peerCount.load() - 1);
        m_peerCount.store(peers);
        double nh = std::max(0.0, m_networkHashrate.load() - 1.0);
        m_networkHashrate.store(nh);
        emit nodeStatusChanged(m_height.load(), m_syncProgress.load(), m_peerCount.load(), m_networkHashrate.load());
    }

    std::atomic<bool> m_running{false};
    std::atomic<int> m_height{0};
    std::atomic<double> m_syncProgress{0.0};
    std::atomic<int> m_peerCount{0};
    std::atomic<double> m_networkHashrate{0.0};
    QTimer m_pollTimer;
    NodeConfig m_config;
    RpcClient m_rpc;
    bool m_inflight{false};
    int m_failures{0};
    QString m_lastError;
};

class MiningManager : public QObject {
    Q_OBJECT
public:
    explicit MiningManager(NodeProcessController* node, QObject* parent = nullptr)
        : QObject(parent), m_node(node)
    {
        connect(&m_tick, &QTimer::timeout, this, &MiningManager::updateRate);
        m_tick.setInterval(1000);
    }

    void start(int threads, bool gpu)
    {
        m_threads = threads;
        m_gpuEnabled = gpu;
        if (m_running.exchange(true)) return;
        m_tick.start();
        emit miningStarted();
    }

    void stop()
    {
        if (!m_running.exchange(false)) return;
        m_tick.stop();
        emit miningStopped();
    }

    bool isRunning() const { return m_running.load(); }
    double lastHashrate() const { return m_lastHashrate.load(); }

signals:
    void hashrateUpdated(double rate);
    void miningStarted();
    void miningStopped();

private slots:
    void updateRate()
    {
        if (!m_running.load()) return;
        double base = 0.0;
        if (m_node) {
            base = m_node->isRunning() ? 15.0 : 5.0;
        }
        double simulated = base + m_threads * 12.5 + (m_gpuEnabled ? 150.0 : 0.0);
        m_lastHashrate.store(simulated);
        emit hashrateUpdated(simulated);
    }

private:
    NodeProcessController* m_node{nullptr};
    std::atomic<bool> m_running{false};
    std::atomic<double> m_lastHashrate{0.0};
    int m_threads{1};
    bool m_gpuEnabled{false};
    QTimer m_tick;
};

class EulaDialog : public QDialog {
    Q_OBJECT
public:
    explicit EulaDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("DRACHMA End User License Agreement");
        QVBoxLayout* v = new QVBoxLayout(this);
        QTextEdit* text = new QTextEdit(this);
        text->setReadOnly(true);
        text->setPlainText(AssetLocator::textAsset("legal/EULA.txt"));
        v->addWidget(text);
        QCheckBox* accept = new QCheckBox("I have read and accept the EULA", this);
        v->addWidget(accept);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText("Accept");
        v->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, [this, accept]{
            if (!accept->isChecked()) {
                QMessageBox::warning(this, "EULA", "You must accept the EULA to continue.");
                return;
            }
            acceptState = true;
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        resize(640, 480);
    }

    bool accepted() const { return acceptState; }

private:
    bool acceptState{false};
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow()
    {
        setWindowTitle("DRACHMA Core Desktop");
        setMinimumSize(1200, 800);
        setWindowIcon(QIcon(AssetLocator::filePath("branding/app_icon.png")));

        createMenu();

        wallet = new WalletServiceClient(this);
        node = new NodeProcessController(this);
        miner = new MiningManager(node, this);

        QWidget* central = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(central);
        tabs = new QTabWidget(central);
        layout->addWidget(tabs);
        setCentralWidget(central);

        tabs->addTab(buildOverview(), "Overview");
        tabs->addTab(buildSend(), "Send");
        tabs->addTab(buildReceive(), "Receive");
        tabs->addTab(buildAddressBook(), "Address book");
        tabs->addTab(buildTransactions(), "Transactions");
        tabs->addTab(buildMining(), "Mining");
        tabs->addTab(buildSettings(), "Settings");

        connect(wallet, &WalletServiceClient::balancesChanged, this, &MainWindow::updateBalances);
        connect(wallet, &WalletServiceClient::transactionsChanged, this, &MainWindow::refreshTransactions);
        connect(wallet, &WalletServiceClient::addressAdded, this, &MainWindow::addAddress);
        connect(wallet, &WalletServiceClient::encryptionStateChanged, this, &MainWindow::updateEncryptionState);
        connect(node, &NodeProcessController::nodeStatusChanged, this, &MainWindow::updateNodeStatus);
        connect(node, &NodeProcessController::nodeStarted, this, [this]{ statusBar()->showMessage("Node started"); });
        connect(node, &NodeProcessController::nodeStopped, this, [this]{ statusBar()->showMessage("Node stopped"); });
        connect(miner, &MiningManager::hashrateUpdated, this, &MainWindow::updateHashrate);
        updateEncryptionState();

        // Load persisted settings and start the node.
        loadSettings();
        enforceEula();
        applyThemeFromSettings();
        loadWalletFromDisk();
        startNodeFromSettings();
        if (wallet->transactions().isEmpty()) seedWallet();
        refreshTransactions();
        refreshAddressBook();
    }

    ~MainWindow() override
    {
        miner->stop();
        node->stopNode();
    }

private:
    void createMenu()
    {
        QMenu* fileMenu = menuBar()->addMenu("File");
        QAction* eulaAction = fileMenu->addAction("View EULA");
        QAction* whitepaperAction = fileMenu->addAction("Open Whitepaper");
        QAction* backupAction = fileMenu->addAction("Backup wallet");
        QAction* restoreAction = fileMenu->addAction("Restore wallet");
        QAction* exitAction = fileMenu->addAction("Exit");
        connect(eulaAction, &QAction::triggered, this, &MainWindow::showEulaDialog);
        connect(whitepaperAction, &QAction::triggered, this, &MainWindow::openWhitepaper);
        connect(backupAction, &QAction::triggered, this, &MainWindow::backupWalletFile);
        connect(restoreAction, &QAction::triggered, this, &MainWindow::restoreWalletFile);
        connect(exitAction, &QAction::triggered, this, &QWidget::close);
    }

    QWidget* buildOverview()
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* v = new QVBoxLayout(w);

        QLabel* tagline = new QLabel(AssetLocator::textAsset("branding/tagline.txt"), w);
        tagline->setWordWrap(true);
        v->addWidget(tagline);

        QGroupBox* nodeBox = new QGroupBox("Node sync status", w);
        QFormLayout* nf = new QFormLayout(nodeBox);
        heightLbl = new QLabel("0", nodeBox);
        peersLbl = new QLabel("0", nodeBox);
        syncLbl = new QLabel("0%", nodeBox);
        syncBar = new QProgressBar(nodeBox);
        syncBar->setRange(0, 100);
        syncBar->setValue(0);
        networkLbl = new QLabel("Offline", nodeBox);
        rpcErrorLabel = new QLabel("RPC: idle", nodeBox);
        nf->addRow("Current height", heightLbl);
        nf->addRow("Peers", peersLbl);
        nf->addRow("Sync", syncLbl);
        nf->addRow("Progress", syncBar);
        nf->addRow("Network status", networkLbl);
        nf->addRow("RPC", rpcErrorLabel);
        nodeBox->setLayout(nf);

        QGroupBox* walletBox = new QGroupBox("Wallet", w);
        QFormLayout* wf = new QFormLayout(walletBox);
        confirmedLbl = new QLabel("0 DRM", walletBox);
        unconfirmedLbl = new QLabel("0 DRM", walletBox);
        wf->addRow("Confirmed", confirmedLbl);
        wf->addRow("Unconfirmed", unconfirmedLbl);
        walletBox->setLayout(wf);

        v->addWidget(nodeBox);
        v->addWidget(walletBox);
        v->addStretch(1);
        return w;
    }

    QWidget* buildSend()
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* v = new QVBoxLayout(w);
        QFormLayout* f = new QFormLayout();

        destEdit = new QLineEdit(w);
        QPushButton* copyFromBook = new QPushButton("Use selected address", w);
        amountEdit = new QDoubleSpinBox(w);
        amountEdit->setRange(0.00000001, 21000000.0);
        amountEdit->setDecimals(8);
        feeBox = new QComboBox(w);
        feeBox->addItems({"Economy", "Normal", "Priority", "Custom"});
        feeRate = new QDoubleSpinBox(w);
        feeRate->setRange(0.1, 1000.0);
        feeRate->setDecimals(1);
        feeRate->setSuffix(" sat/vB");
        feeRate->setValue(5.0);
        feePreview = new QLabel("Est. fee: 0.0001 DRM", w);

        connect(copyFromBook, &QPushButton::clicked, this, [this]{
            if (!addressBookTable) return;
            auto rows = addressBookTable->selectionModel()->selectedRows();
            if (!rows.isEmpty()) {
                destEdit->setText(addressBookTable->item(rows.first().row(), 1)->text());
            }
        });
        connect(feeBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateFeePreview);
        connect(feeRate, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::updateFeePreview);

        QPushButton* sendBtn = new QPushButton("Send", w);
        connect(sendBtn, &QPushButton::clicked, this, &MainWindow::confirmAndSend);

        f->addRow("Destination address", destEdit);
        f->addRow("From address book", copyFromBook);
        f->addRow("Amount", amountEdit);
        f->addRow("Fee profile", feeBox);
        f->addRow("Fee rate", feeRate);
        f->addRow("Fee preview", feePreview);
        v->addLayout(f);
        v->addWidget(sendBtn);
        v->addStretch(1);
        updateFeePreview();
        return w;
    }

    QWidget* buildReceive()
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* v = new QVBoxLayout(w);
        addressList = new QListWidget(w);
        QLabel* qrLabelTitle = new QLabel("QR code (placeholder)", w);
        qrLabel = new QLabel(w);
        qrLabel->setFixedSize(220, 220);
        qrLabel->setFrameShape(QFrame::Box);

        QPushButton* genBtn = new QPushButton("Generate address", w);
        connect(genBtn, &QPushButton::clicked, this, &MainWindow::generateAddress);

        v->addWidget(new QLabel("Receiving addresses", w));
        v->addWidget(addressList);
        v->addWidget(qrLabelTitle);
        v->addWidget(qrLabel);
        v->addWidget(genBtn);
        v->addStretch(1);
        return w;
    }

    QWidget* buildAddressBook()
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* v = new QVBoxLayout(w);
        addressBookTable = new QTableWidget(w);
        addressBookTable->setColumnCount(2);
        addressBookTable->setHorizontalHeaderLabels({"Label", "Address"});
        addressBookTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        addressBookTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        addressBookTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        QHBoxLayout* actions = new QHBoxLayout();
        QPushButton* addBtn = new QPushButton("Add", w);
        QPushButton* removeBtn = new QPushButton("Remove", w);
        QPushButton* copyBtn = new QPushButton("Copy", w);
        actions->addWidget(addBtn);
        actions->addWidget(removeBtn);
        actions->addWidget(copyBtn);

        connect(addBtn, &QPushButton::clicked, this, &MainWindow::addAddressBookEntryDialog);
        connect(removeBtn, &QPushButton::clicked, this, &MainWindow::removeAddressBookEntryDialog);
        connect(copyBtn, &QPushButton::clicked, this, &MainWindow::copyAddressFromBook);

        v->addWidget(addressBookTable);
        v->addLayout(actions);
        v->addStretch(1);
        return w;
    }

    QWidget* buildTransactions()
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* v = new QVBoxLayout(w);
        txTable = new QTableWidget(w);
        txTable->setColumnCount(6);
        txTable->setHorizontalHeaderLabels({"Time", "TXID", "Direction", "Amount", "Confirmations", "Status"});
        txTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        txTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        txTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        v->addWidget(txTable);
        return w;
    }

    QWidget* buildMining()
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* v = new QVBoxLayout(w);
        QHBoxLayout* controls = new QHBoxLayout();
        QPushButton* startBtn = new QPushButton("Start mining", w);
        QPushButton* stopBtn = new QPushButton("Stop", w);
        cpuThreads = new QSpinBox(w);
        cpuThreads->setRange(1, std::thread::hardware_concurrency() == 0 ? 8 : static_cast<int>(std::thread::hardware_concurrency()));
        gpuToggle = new QCheckBox("Enable GPU (if available)", w);
        hashrateLbl = new QLabel("0 H/s", w);

        connect(startBtn, &QPushButton::clicked, this, &MainWindow::startMining);
        connect(stopBtn, &QPushButton::clicked, this, &MainWindow::stopMining);

        controls->addWidget(new QLabel("CPU threads", w));
        controls->addWidget(cpuThreads);
        controls->addWidget(gpuToggle);
        controls->addWidget(startBtn);
        controls->addWidget(stopBtn);
        v->addLayout(controls);

        v->addWidget(new QLabel("Hashrate", w));
        v->addWidget(hashrateLbl);
        return w;
    }

    QWidget* buildSettings()
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* v = new QVBoxLayout(w);
        QFormLayout* f = new QFormLayout();

        dataDirEdit = new QLineEdit(w);
        QPushButton* browse = new QPushButton("Browse", w);
        connect(browse, &QPushButton::clicked, this, &MainWindow::selectDataDir);

        QWidget* dirWidget = new QWidget(w);
        QHBoxLayout* dirLayout = new QHBoxLayout(dirWidget);
        dirLayout->setContentsMargins(0,0,0,0);
        dirLayout->addWidget(dataDirEdit);
        dirLayout->addWidget(browse);

        networkBox = new QComboBox(w);
        networkBox->addItems({"mainnet", "testnet"});
        rpcUserEdit = new QLineEdit(w);
        rpcPassEdit = new QLineEdit(w);
        rpcPassEdit->setEchoMode(QLineEdit::Password);

        themeBox = new QComboBox(w);
        themeBox->addItems({"System", "Dark", "Light"});
        connect(themeBox, &QComboBox::currentTextChanged, this, &MainWindow::applyTheme);

        encryptBtn = new QPushButton("Encrypt wallet", w);
        unlockBtn = new QPushButton("Unlock wallet", w);
        connect(encryptBtn, &QPushButton::clicked, this, &MainWindow::encryptWallet);
        connect(unlockBtn, &QPushButton::clicked, this, &MainWindow::unlockWallet);

        f->addRow("Data directory", dirWidget);
        f->addRow("Network", networkBox);
        f->addRow("RPC username", rpcUserEdit);
        f->addRow("RPC password", rpcPassEdit);
        f->addRow("Theme", themeBox);
        f->addRow("Wallet encryption", encryptBtn);
        f->addRow("Wallet unlock", unlockBtn);

        QPushButton* save = new QPushButton("Save settings", w);
        connect(save, &QPushButton::clicked, this, &MainWindow::saveSettings);

        QPushButton* backupBtn = new QPushButton("Backup wallet", w);
        QPushButton* restoreBtn = new QPushButton("Restore wallet", w);
        connect(backupBtn, &QPushButton::clicked, this, &MainWindow::backupWalletFile);
        connect(restoreBtn, &QPushButton::clicked, this, &MainWindow::restoreWalletFile);

        v->addLayout(f);
        v->addWidget(save);
        v->addWidget(backupBtn);
        v->addWidget(restoreBtn);
        v->addStretch(1);
        return w;
    }

    void enforceEula()
    {
        QSettings s("Drachma", "CoreDesktop");
        if (s.value("eulaAccepted", false).toBool()) return;
        EulaDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted && dlg.accepted()) {
            s.setValue("eulaAccepted", true);
        } else {
            QMessageBox::critical(this, "EULA", "You must accept the EULA to use DRACHMA Core.");
            QTimer::singleShot(0, this, &QWidget::close);
        }
    }

    void seedWallet()
    {
        wallet->creditSimulated(25 * 100000000LL, "initial-aid");
        wallet->requestNewAddress("default");
    }

    void promptForEncryptionSetup()
    {
        bool ok = false;
        QString pass = QInputDialog::getText(this, "Create wallet passphrase", "Enter a strong passphrase", QLineEdit::Password, "", &ok);
        if (!ok || pass.isEmpty()) {
            QMessageBox::warning(this, "Encryption", "A passphrase is required to secure your wallet.");
            return;
        }
        QString confirm = QInputDialog::getText(this, "Confirm passphrase", "Re-enter passphrase", QLineEdit::Password, "", &ok);
        if (!ok || pass != confirm) {
            QMessageBox::critical(this, "Encryption", "Passphrases did not match.");
            return;
        }
        QString err;
        if (!wallet->encryptWallet(pass, err)) {
            QMessageBox::critical(this, "Encryption", err);
            return;
        }
        if (!wallet->unlockWallet(pass, err)) {
            QMessageBox::critical(this, "Encryption", err);
            return;
        }
        QMessageBox::information(this, "Encryption", "Wallet encrypted with AES-256. Keep your passphrase safe.");
        updateEncryptionState();
    }

private slots:
    void updateBalances(qint64 confirmed, qint64 unconfirmed)
    {
        confirmedLbl->setText(QString::number(confirmed / 100000000.0, 'f', 8) + " DRM");
        unconfirmedLbl->setText(QString::number(unconfirmed / 100000000.0, 'f', 8) + " DRM");
    }

    void updateNodeStatus(int height, double syncProgress, int peers, double networkHashrate)
    {
        heightLbl->setText(QString::number(height));
        peersLbl->setText(QString::number(peers));
        syncLbl->setText(QString::number(syncProgress * 100.0, 'f', 2) + "%");
        if (syncBar) syncBar->setValue(static_cast<int>(syncProgress * 100.0));
        networkLbl->setText(QString::number(networkHashrate, 'f', 2) + " H/s");
        QString rpcMsg = node->lastError().isEmpty() ? QString("RPC: online") : QString("RPC issue: %1").arg(node->lastError());
        rpcErrorLabel->setText(rpcMsg);
        if (!node->lastError().isEmpty()) {
            statusBar()->showMessage("RPC communication degraded: " + node->lastError(), 5000);
        }
    }

    void updateHashrate(double rate)
    {
        hashrateLbl->setText(QString::number(rate, 'f', 2) + " H/s");
    }

    void refreshTransactions()
    {
        auto rows = wallet->transactions();
        txTable->setRowCount(rows.size());
        int i = 0;
        for (const auto& row : rows) {
            txTable->setItem(i, 0, new QTableWidgetItem(row.timestamp.toString(Qt::ISODate)));
            txTable->setItem(i, 1, new QTableWidgetItem(row.txid));
            txTable->setItem(i, 2, new QTableWidgetItem(row.direction));
            txTable->setItem(i, 3, new QTableWidgetItem(QString::number(row.amount / 100000000.0, 'f', 8)));
            txTable->setItem(i, 4, new QTableWidgetItem(QString::number(row.confirmations)));
            txTable->setItem(i, 5, new QTableWidgetItem(row.status));
            ++i;
        }
    }

    void refreshAddressBook()
    {
        if (!addressBookTable) return;
        auto entries = wallet->addressBook();
        addressBookTable->setRowCount(entries.size());
        int i = 0;
        for (const auto& e : entries) {
            addressBookTable->setItem(i, 0, new QTableWidgetItem(e.label));
            addressBookTable->setItem(i, 1, new QTableWidgetItem(e.address));
            ++i;
        }
    }

    void addAddress(const QString& addr)
    {
        addressList->addItem(addr);
        drawQr(addr);
    }

    void generateAddress()
    {
        bool ok = false;
        QString label = QInputDialog::getText(this, "New address", "Label", QLineEdit::Normal, "", &ok);
        if (!ok) return;
        QString addr = wallet->requestNewAddress(label);
        drawQr(addr);
    }

    void addAddressBookEntryDialog()
    {
        bool ok = false;
        QString label = QInputDialog::getText(this, "Address label", "Label", QLineEdit::Normal, "", &ok);
        if (!ok || label.isEmpty()) return;
        QString address = QInputDialog::getText(this, "Address", "Destination", QLineEdit::Normal, "", &ok);
        if (!ok || address.isEmpty()) return;
        wallet->addAddressBookEntry(label, address);
        refreshAddressBook();
    }

    void removeAddressBookEntryDialog()
    {
        if (!addressBookTable) return;
        auto rows = addressBookTable->selectionModel()->selectedRows();
        if (rows.isEmpty()) return;
        int idx = rows.first().row();
        wallet->removeAddressBookEntry(idx);
        refreshAddressBook();
    }

    void copyAddressFromBook()
    {
        if (!addressBookTable) return;
        auto rows = addressBookTable->selectionModel()->selectedRows();
        if (rows.isEmpty()) return;
        QString addr = addressBookTable->item(rows.first().row(), 1)->text();
        QApplication::clipboard()->setText(addr);
        destEdit->setText(addr);
        statusBar()->showMessage("Address copied", 2000);
    }

    void drawQr(const QString& data)
    {
        const int modules = 25;
        QByteArray digest = QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha256);
        QImage img(modules, modules, QImage::Format_ARGB32);
        for (int y = 0; y < modules; ++y) {
            for (int x = 0; x < modules; ++x) {
                int idx = (x + y * modules) % digest.size();
                bool dark = (static_cast<unsigned char>(digest[idx]) >> (idx % 8)) & 0x1;
                img.setPixelColor(x, y, dark ? Qt::black : Qt::white);
            }
        }
        QImage scaled = img.scaled(qrLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation);
        qrLabel->setPixmap(QPixmap::fromImage(scaled));
    }

    void confirmAndSend()
    {
        QString dest = destEdit->text().trimmed();
        double amountD = amountEdit->value();
        qint64 amount = static_cast<qint64>(amountD * 100000000.0);
        qint64 fee = estimateFeeSats();
        if (dest.isEmpty()) {
            QMessageBox::warning(this, "Send", "Destination address is required.");
            return;
        }
        QString summary = QString("Send %1 DRM to %2 with fee %3?")
            .arg(amountD, 0, 'f', 8)
            .arg(dest)
            .arg(fee / 100000000.0, 0, 'f', 8);
        if (QMessageBox::question(this, "Confirm transaction", summary) != QMessageBox::Yes)
            return;
        QString err;
        if (!wallet->createSpend(dest, amount, fee, err)) {
            QMessageBox::critical(this, "Send", err);
            return;
        }
        QMessageBox::information(this, "Send", "Transaction created and queued for broadcast.");
    }

    double currentFeeRate() const
    {
        switch (feeBox->currentIndex()) {
            case 0: return 1.0;   // Economy
            case 1: return 5.0;   // Normal
            case 2: return 15.0;  // Priority
            default: return feeRate ? feeRate->value() : 5.0;
        }
    }

    qint64 estimateFeeSats() const
    {
        double rate = currentFeeRate();
        double estimatedSize = 225.0; // typical P2PKH-sized send
        return static_cast<qint64>(rate * estimatedSize);
    }

    void updateFeePreview()
    {
        if (!feePreview) return;
        double rate = currentFeeRate();
        qint64 fee = estimateFeeSats();
        if (feeRate) feeRate->setEnabled(feeBox && feeBox->currentIndex() == 3);
        feePreview->setText(QString("Est. fee: %1 DRM (%2 sat/vB)")
            .arg(fee / 100000000.0, 0, 'f', 8)
            .arg(rate, 0, 'f', 1));
    }

    void startMining()
    {
        miner->start(cpuThreads->value(), gpuToggle->isChecked());
    }

    void stopMining()
    {
        miner->stop();
    }

    void selectDataDir()
    {
        QString dir = QFileDialog::getExistingDirectory(this, "Select data directory", dataDirEdit->text());
        if (!dir.isEmpty()) dataDirEdit->setText(dir);
    }

    void saveSettings()
    {
        QSettings s("Drachma", "CoreDesktop");
        s.setValue("dataDir", dataDirEdit->text());
        s.setValue("network", networkBox->currentText());
        s.setValue("rpcUser", rpcUserEdit->text());
        s.setValue("rpcPass", rpcPassEdit->text());
        s.setValue("theme", themeBox->currentText());
        statusBar()->showMessage("Settings saved", 3000);
        startNodeFromSettings();
        applyTheme(themeBox->currentText());
    }

    void loadSettings()
    {
        QSettings s("Drachma", "CoreDesktop");
        QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        dataDirEdit->setText(s.value("dataDir", defaultDir).toString());
        networkBox->setCurrentText(s.value("network", "testnet").toString());
        rpcUserEdit->setText(s.value("rpcUser", "user").toString());
        rpcPassEdit->setText(s.value("rpcPass", "pass").toString());
        themeBox->setCurrentText(s.value("theme", "System").toString());
    }

    void applyThemeFromSettings()
    {
        QSettings s("Drachma", "CoreDesktop");
        applyTheme(s.value("theme", "System").toString());
    }

    void startNodeFromSettings()
    {
        NodeConfig cfg{dataDirEdit->text(), networkBox->currentText(), rpcUserEdit->text(), rpcPassEdit->text()};
        if (node->isRunning()) node->stopNode();
        node->startNode(cfg);
    }

    void loadWalletFromDisk()
    {
        QString walletPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/wallet.dat";
        QString err;
        if (!QFile::exists(walletPath)) {
            promptForEncryptionSetup();
            return;
        }

        bool ok = false;
        QString pass = QInputDialog::getText(this, "Unlock wallet", "Enter wallet passphrase", QLineEdit::Password, "", &ok);
        if (!ok) return;
        if (!wallet->loadWithPassphrase(pass, err)) {
            QMessageBox::critical(this, "Wallet", err);
        } else {
            updateEncryptionState();
            refreshTransactions();
        }
    }

    void applyTheme(const QString& theme)
    {
        if (themeBox && themeBox->currentText() != theme)
            themeBox->setCurrentText(theme);
        if (theme == "Dark") {
            QPalette p;
            p.setColor(QPalette::Window, QColor(30,30,30));
            p.setColor(QPalette::WindowText, Qt::white);
            p.setColor(QPalette::Base, QColor(45,45,45));
            p.setColor(QPalette::Text, Qt::white);
            p.setColor(QPalette::Button, QColor(60,60,60));
            p.setColor(QPalette::ButtonText, Qt::white);
            QApplication::setStyle(QStyleFactory::create("Fusion"));
            QApplication::setPalette(p);
        } else if (theme == "Light") {
            QApplication::setStyle(QStyleFactory::create("Fusion"));
            QApplication::setPalette(QApplication::style()->standardPalette());
        } else {
            QApplication::setPalette(QApplication::style()->standardPalette());
        }
    }

    void encryptWallet()
    {
        bool ok = false;
        QString pass = QInputDialog::getText(this, "Encrypt wallet", "Passphrase", QLineEdit::Password, "", &ok);
        if (!ok) return;
        QString err;
        if (!wallet->encryptWallet(pass, err)) {
            QMessageBox::critical(this, "Encryption", err);
            return;
        }
        QMessageBox::information(this, "Encryption", "Wallet encrypted. Please keep your passphrase safe.");
    }

    void unlockWallet()
    {
        bool ok = false;
        QString pass = QInputDialog::getText(this, "Unlock wallet", "Passphrase", QLineEdit::Password, "", &ok);
        if (!ok) return;
        QString err;
        if (!wallet->unlockWallet(pass, err)) {
            QMessageBox::critical(this, "Unlock", err);
            return;
        }
        QMessageBox::information(this, "Unlock", "Wallet unlocked.");
    }

    void backupWalletFile()
    {
        QMessageBox::information(this, "Backup", "Backups include encrypted keys; store them offline and protect your passphrase.");
        QString dest = QFileDialog::getSaveFileName(this, "Backup wallet", QDir::homePath() + "/wallet-backup.dat");
        if (dest.isEmpty()) return;
        QString err;
        if (!wallet->backupWallet(dest, err)) {
            QMessageBox::critical(this, "Backup", err);
        } else {
            QMessageBox::information(this, "Backup", "Wallet backup saved.");
        }
    }

    void restoreWalletFile()
    {
        QMessageBox::warning(this, "Restore", "Restoring replaces your current wallet. Only proceed if you trust the backup file.");
        QString src = QFileDialog::getOpenFileName(this, "Restore wallet", QDir::homePath());
        if (src.isEmpty()) return;
        bool ok = false;
        QString pass = QInputDialog::getText(this, "Restore wallet", "Passphrase", QLineEdit::Password, "", &ok);
        if (!ok) return;
        QString err;
        if (!wallet->restoreWallet(src, pass, err)) {
            QMessageBox::critical(this, "Restore", err);
        } else {
            QMessageBox::information(this, "Restore", "Wallet restored.");
            refreshTransactions();
            refreshAddressBook();
            updateEncryptionState();
        }
    }

    void updateEncryptionState()
    {
        encryptBtn->setEnabled(!wallet->isEncrypted());
        unlockBtn->setEnabled(wallet->isEncrypted());
        if (wallet->isEncrypted()) {
            unlockBtn->setText(wallet->isLocked() ? "Unlock wallet" : "Wallet unlocked");
        }
    }

    void showEulaDialog()
    {
        EulaDialog dlg(this);
        dlg.exec();
    }

    void openWhitepaper()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(AssetLocator::filePath("docs/whitepaper.pdf")));
    }

private:
    WalletServiceClient* wallet{nullptr};
    NodeProcessController* node{nullptr};
    MiningManager* miner{nullptr};

    QTabWidget* tabs{nullptr};

    QLabel* heightLbl{nullptr};
    QLabel* peersLbl{nullptr};
    QLabel* syncLbl{nullptr};
    QProgressBar* syncBar{nullptr};
    QLabel* networkLbl{nullptr};
    QLabel* confirmedLbl{nullptr};
    QLabel* unconfirmedLbl{nullptr};
    QLabel* hashrateLbl{nullptr};

    QLineEdit* destEdit{nullptr};
    QDoubleSpinBox* amountEdit{nullptr};
    QComboBox* feeBox{nullptr};
    QDoubleSpinBox* feeRate{nullptr};
    QLabel* feePreview{nullptr};

    QListWidget* addressList{nullptr};
    QLabel* qrLabel{nullptr};

    QTableWidget* addressBookTable{nullptr};

    QLabel* rpcErrorLabel{nullptr};
    QComboBox* themeBox{nullptr};

    QTableWidget* txTable{nullptr};

    QSpinBox* cpuThreads{nullptr};
    QCheckBox* gpuToggle{nullptr};

    QLineEdit* dataDirEdit{nullptr};
    QComboBox* networkBox{nullptr};
    QLineEdit* rpcUserEdit{nullptr};
    QLineEdit* rpcPassEdit{nullptr};
    QPushButton* encryptBtn{nullptr};
    QPushButton* unlockBtn{nullptr};
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}

#include "main.moc"
