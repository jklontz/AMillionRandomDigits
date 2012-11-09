#include <QBitArray>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QVector>

struct Number
{
    uint value, bits;
    Number() : value(0), bits(0) {}
    Number(uint _value, uint _bits) : value(_value), bits(_bits) {}
    Number(const QBitArray &bitArray)
    {
        bits = bitArray.size();
        value = 0;
        for (int i=0; i<bits; i++)
            if (bitArray.at(i)) value += 1 << (bits-i-1);
    }

    operator QBitArray() const
    {
        QBitArray result(bits);
        for (int i=0; i<bits; i++)
            result.setBit(value & (1 << bits-i-1), i);
        return result;
    }

    inline bool operator[](int index) const
    {
        return value & (1 << (bits-index-1));
    }

    inline operator uint() const
    {
        return value;
    }

    inline void shift(bool nextVal)
    {
        value = ((value << 1) & ((ulong(1) << bits) - 1)) + (nextVal ? 1 : 0);
    }

    inline Number &operator<<(bool nextVal)
    {
        shift(nextVal);
        return *this;
    }
};

QDebug operator<<(QDebug dbg, const Number &n)
{
    dbg.nospace() << n.value << " (" << n.bits << ")";
    return dbg.space();
}

struct Count
{
    Number number;
    int count;
    Count() : count(-1) {}
    Count(const Number &_number, uint _count) : number(_number), count(_count) {}
};

static void count(const QBitArray &data, uint bits, Count &most, Count &least)
{
    Number index(0, bits);
    for (int i=0; i<bits-1; i++)
        index << data.at(i);

    const uint size = (ulong(1) << bits) - 1;
    uchar *counts = new uchar[size];
    memset(counts, 0, size);

    for (uint i=0; i<data.size()-bits; i++) {
        index << data.at(i + bits - 1);
        if (counts[index] < 255) counts[index]++;
    }

    most = Count(Number(), 0);
    least = Count(Number(), 255);
    for (uint i=0; i<size; i++) {
        if (counts[i] > most.count) most = Count(Number(i, bits), counts[i]);
        if (counts[i] < least.count) least = Count(Number(i, bits), counts[i]);
    }

    delete[] counts;
}

static Count mostCommon(const QBitArray &data, int bits)
{
    Count most, least;
    count(data, bits, most, least);
    return most;
}

static Count leastCommon(const QBitArray &data, int bits)
{
    Count most, least;
    count(data, bits, most, least);
    return least;
}

static QBitArray compress(const QBitArray &data, const Number &before, const Number &after)
{
    const int bits = before.bits;
    Number beforeSize(before.bits, 5);
    Number afterSize(after.bits, 5);
    QBitArray result(data.size() + beforeSize.bits + before.bits);

    int resultIndex = 0;
    for (int i=0; i<beforeSize.bits; i++)
        result.setBit(resultIndex + i, beforeSize[i]);
    resultIndex += beforeSize.bits;
    for (int i=0; i<before.bits; i++)
        result.setBit(resultIndex + i, before[i]);
    resultIndex += before.bits;
    for (int i=0; i<afterSize.bits; i++)
        result.setBit(resultIndex + i, afterSize[i]);
    resultIndex += afterSize.bits;
    for (int i=0; i<after.bits; i++)
        result.setBit(resultIndex + i, after[i]);
    resultIndex += after.bits;

    Number index(0, bits);
    for (int i=0; i<bits-1; i++)
        index << data.at(i);

    int i = 0;
    while (i < data.size()-bits) {
        index << data.at(i + bits - 1);
        if (index == before) {
            for (int j=0; j<after.bits; j++)
                result.setBit(resultIndex+j, after[j]);
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
    uint j = 0;
    Count least;
    while (least.count != 0) {
        j++;
        least = leastCommon(data, j);
    }

    int closest = data.size();
    for (int i=32; i>j; i--) {
        Count most = mostCommon(data, i);
        QBitArray compressed = compress(data, most.number, least.number);
        if (compressed.size() < data.size())
            return compressed;
        closest = std::min(closest, compressed.size()-data.size());
    }

    qDebug("Could not compress, got as close as %d :(", closest);
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
