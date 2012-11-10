#include <QBitArray>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QVector>

struct Number
{
    quint64 value;
    quint8 bits;
    Number() : value(0), bits(0) {}
    Number(quint64 _value, quint8 _bits) : value(_value), bits(_bits) {}
    Number(const QBitArray &bitArray)
    {
        bits = bitArray.size();
        value = 0;
        for (quint8 i=0; i<bits; i++)
            if (bitArray.at(i)) value += 1 << (bits-i-1);
    }

    inline bool operator[](int index) const { return value & (1 << (bits-index-1)); }
    inline operator uint() const { return value; }
    inline void shift(bool nextVal) { value = ((value << 1) & ((quint64(1) << bits) - 1)) + nextVal; }
    inline Number &operator<<(bool nextVal) { shift(nextVal); return *this; }
};

QDebug operator<<(QDebug dbg, const Number &n)
{
    dbg.nospace() << n.value << " (" << n.bits << ")";
    return dbg.space();
}

struct Count
{
    Number number;
    qint32 count;
    Count() : count(-1) {}
    Count(const Number &_number, qint32 _count) : number(_number), count(_count) {}
};

static void count(const QBitArray &data, quint8 bits, Count &most, Count &least)
{
    Number index(0, bits);
    for (quint8 i=0; i<bits-1; i++)
        index << data.at(i);

    const quint64 size = quint64(1) << bits;
    quint8 *counts = new quint8[size];
    memset(counts, 0, size);

    for (quint64 i=0; i<data.size()-bits; i++) {
        index << data.at(i + bits - 1);
        if (counts[index] < 255) counts[index]++;
    }

    most = Count(Number(), 0);
    least = Count(Number(), 255);
    for (quint64 i=0; i<size; i++) {
        if (counts[i] > most.count) most = Count(Number(i, bits), counts[i]);
        if (counts[i] < least.count) least = Count(Number(i, bits), counts[i]);
    }

    delete[] counts;
}

static Count leastCommon(const QBitArray &data)
{
    uint bits = 0;
    Count most, least;
    while (least.count != 0) {
        bits++;
        count(data, bits, most, least);
    }
    return least;
}

static QBitArray compress(const QBitArray &data, const Number &before, const Number &after)
{
    const int bits = before.bits;
    Number beforeSize(before.bits, 5);
    Number afterSize(after.bits, 5);
    QBitArray result(data.size() + beforeSize.bits + before.bits);

    quint64 resultIndex = 0;
    for (quint8 i=0; i<beforeSize.bits; i++) result.setBit(resultIndex + i, beforeSize[i]);
    resultIndex += beforeSize.bits;
    for (quint8 i=0; i<before.bits; i++) result.setBit(resultIndex + i, before[i]);
    resultIndex += before.bits;
    for (quint8 i=0; i<afterSize.bits; i++) result.setBit(resultIndex + i, afterSize[i]);
    resultIndex += afterSize.bits;
    for (quint8 i=0; i<after.bits; i++) result.setBit(resultIndex + i, after[i]);
    resultIndex += after.bits;

    Number index(0, bits);
    for (quint8 i=0; i<bits-1; i++)
        index << data.at(i);

    quint64 i = 0;
    while (i < data.size()-bits) {
        index << data.at(i + bits - 1);
        if (index == before) {
            for (quint8 j=0; j<after.bits; j++) result.setBit(resultIndex+j, after[j]);
            resultIndex += after.bits;
            i += before.bits;
        } else {
            result.setBit(resultIndex++, data.at(i++));
        }
    }
    while (i < data.size())
        result.setBit(resultIndex++, data.at(i++));

    result.resize(resultIndex);
    return result;
}

static QBitArray tryCompress(const QBitArray &data)
{
    qint32 closest = data.size();
    for (quint8 i=32; i>0; i--) {
        Count most, least;
        count(data, i, most, least);
        least = leastCommon(compress(data, most.number, Number()));
        if (least.number.bits >= most.number.bits) break;
        QBitArray compressed = compress(data, most.number, least.number);
        if (compressed.size() < data.size()) return compressed;
        closest = qMin(closest, compressed.size()-data.size());
    }

    qDebug("Could not compress, got as close as %d bits", closest);
    return data;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("compress <file>\n");
        return 1;
    }

    QFile file(argv[1]);
    file.open(QFile::ReadOnly);
    QByteArray byteArray = file.readAll();
    file.close();

    QBitArray data(8*byteArray.size());
    for (int i=0; i<byteArray.size(); i++)
        for (int j=0; j<8; j++)
            data.setBit(i*8+j, byteArray[i] & (1 << j));

    QBitArray result = tryCompress(data);
    qDebug("Size change = %d", data.size() - result.size());

    return 0;
}
