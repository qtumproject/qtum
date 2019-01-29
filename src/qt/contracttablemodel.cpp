#include <qt/contracttablemodel.h>

#include <qt/guiutil.h>
#include <qt/walletmodel.h>

#include <interfaces/wallet.h>

#include <QFont>
#include <QDebug>
#include <utility>

struct ContractTableEntry
{
    QString label;
    QString address;
    QString abi;

    ContractTableEntry() {}
    ContractTableEntry(const QString &_label, const QString &_address, const QString &_abi):
        label(_label), address(_address), abi(_abi) {}
};

struct ContractTableEntryLessThan
{
    bool operator()(const ContractTableEntry &a, const ContractTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const ContractTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const ContractTableEntry &b) const
    {
        return a < b.address;
    }
};

// Private implementation
class ContractTablePriv
{
public:
    QList<ContractTableEntry> cachedContractTable;
    ContractTableModel *parent;

    ContractTablePriv(ContractTableModel *_parent):
        parent(_parent) {}

    void refreshContractTable(interfaces::Wallet& wallet)
    {
        cachedContractTable.clear();
        for(interfaces::ContractBookData item : wallet.getContractBooks())
        {
            cachedContractTable.append(ContractTableEntry(
                              QString::fromStdString(item.name),
                              QString::fromStdString(item.address),
                              QString::fromStdString(item.abi)));
        }

        // qLowerBound() and qUpperBound() require our cachedContractTable list to be sorted in asc order
        qSort(cachedContractTable.begin(), cachedContractTable.end(), ContractTableEntryLessThan());
    }

    void updateEntry(const QString &address, const QString &label, const QString &abi, int status)
    {
        // Find address / label in model
        QList<ContractTableEntry>::iterator lower = qLowerBound(
            cachedContractTable.begin(), cachedContractTable.end(), address, ContractTableEntryLessThan());
        QList<ContractTableEntry>::iterator upper = qUpperBound(
            cachedContractTable.begin(), cachedContractTable.end(), address, ContractTableEntryLessThan());
        int lowerIndex = (lower - cachedContractTable.begin());
        int upperIndex = (upper - cachedContractTable.begin());
        bool inModel = (lower != upper);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "ContractTablePriv::updateEntry: Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedContractTable.insert(lowerIndex, ContractTableEntry(label, address, abi));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "ContractTablePriv::updateEntry: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            lower->label = label;
            lower->abi = abi;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "ContractTablePriv::updateEntry: Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedContractTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedContractTable.size();
    }

    ContractTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedContractTable.size())
        {
            return &cachedContractTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

ContractTableModel::ContractTableModel(WalletModel *parent) :
    QAbstractTableModel(parent),walletModel(parent),priv(0)
{
    columns << tr("Label") << tr("Contract Address") << tr("Interface (ABI)");
    priv = new ContractTablePriv(this);
    priv->refreshContractTable(walletModel->wallet());
}

ContractTableModel::~ContractTableModel()
{
    delete priv;
}

int ContractTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int ContractTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ContractTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    ContractTableEntry *rec = static_cast<ContractTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Label:
            if(rec->label.isEmpty() && role == Qt::DisplayRole)
            {
                return tr("(no label)");
            }
            else
            {
                return rec->label;
            }
        case Address:
            return rec->address;
        case ABI:
            return rec->abi;
        }
    }
    else if (role == Qt::FontRole)
    {
        QFont font;
        if(index.column() == Address)
        {
            font = GUIUtil::fixedPitchFont();
        }
        return font;
    }

    return QVariant();
}

bool ContractTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    ContractTableEntry *rec = static_cast<ContractTableEntry*>(index.internalPointer());

    if(role == Qt::EditRole)
    {
        std::string curAddress = rec->address.toStdString();
        std::string curLabel = rec->label.toStdString();
        std::string curAbi = rec->abi.toStdString();
        if(index.column() == Label)
        {
            // Do nothing, if old label == new label
            if(rec->label == value.toString())
            {
                updateEditStatus(NO_CHANGES);
                return false;
            }
            walletModel->wallet().setContractBook(curAddress, value.toString().toStdString(), curAbi);
        } else if(index.column() == Address) {
            std::string newAddress = value.toString().toStdString();

            // Do nothing, if old address == new address
            if(newAddress == curAddress)
            {
                updateEditStatus(NO_CHANGES);
                return false;
            }
            // Check for duplicate addresses to prevent accidental deletion of addresses, if you try
            // to paste an existing address over another address (with a different label)
            else if(walletModel->wallet().existContractBook(newAddress))
            {
                updateEditStatus(DUPLICATE_ADDRESS);
                return false;
            }
            else
            {
                // Remove old entry
                walletModel->wallet().delContractBook(curAddress);
                // Add new entry with new address
                walletModel->wallet().setContractBook(newAddress, curLabel, curAbi);
            }
        }
        else if(index.column() == ABI) {
            // Do nothing, if old abi == new abi
            if(rec->abi == value.toString())
            {
                updateEditStatus(NO_CHANGES);
                return false;
            }
            walletModel->wallet().setContractBook(curAddress, curLabel, value.toString().toStdString());
        }
        return true;
    }
    return false;
}

QVariant ContractTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

QModelIndex ContractTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    ContractTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void ContractTableModel::updateEntry(const QString &address,
        const QString &label, const QString &abi, int status)
{
    // Update contract book model from Qtum core
    priv->updateEntry(address, label, abi, status);
}

QString ContractTableModel::addRow(const QString &label, const QString &address, const QString &abi)
{
    // Check for duplicate entry
    if(lookupAddress(address) != -1)
    {
        editStatus = DUPLICATE_ADDRESS;
        return "";
    }

    // Add new entry
    std::string strLabel = label.toStdString();
    std::string strAddress = address.toStdString();
    std::string strAbi = abi.toStdString();
    editStatus = OK;
    walletModel->wallet().setContractBook(strAddress, strLabel, strAbi);
    return address;
}

bool ContractTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    ContractTableEntry *rec = priv->index(row);
    if(count != 1 || !rec )
    {
        // Can only remove one row at a time, and cannot remove rows not in model.
        return false;
    }
    walletModel->wallet().delContractBook(rec->address.toStdString());
    return true;
}

/* Label for address in contract book, if not found return empty string.
 */
QString ContractTableModel::labelForAddress(const QString &address) const
{
    interfaces::ContractBookData item = walletModel->wallet().getContractBook(address.toStdString());
    return QString::fromStdString(item.name);
}

/* ABI for address in contract book, if not found return empty string.
 */
QString ContractTableModel::abiForAddress(const QString &address) const
{
    interfaces::ContractBookData item = walletModel->wallet().getContractBook(address.toStdString());
    return QString::fromStdString(item.abi);
}

int ContractTableModel::lookupAddress(const QString &address) const
{
    QModelIndexList lst = match(index(0, Address, QModelIndex()),
                                Qt::EditRole, address, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void ContractTableModel::resetEditStatus()
{
    editStatus = OK;
}

void ContractTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

void ContractTableModel::updateEditStatus(ContractTableModel::EditStatus status)
{
    if(status > editStatus)
    {
        editStatus = status;
    }
}
