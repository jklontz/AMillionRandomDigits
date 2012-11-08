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

    inline operator uint() const
    {
        return value;
    }

    inline void shift(bool nextVal)
    {
        value = ((value << 1) & ((1 << bits) - 1)) + (nextVal ? 1 : 0);
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

static void search(const QBitArray &data, uint bits, Count &most, Count &least)
{
    Number index(0, bits);
    for (int i=0; i<bits-1; i++)
        index << data.at(i);

    QVector<int> counts(1 << bits, 0);
    for (int i=0; i<data.size()-bits; i++) {
        index << data.at(i + bits - 1);
        counts[index]++;
    }

    most = Count(Number(), 0);
    least = Count(Number(), data.size());
    for (int i=0; i<counts.size(); i++) {
        if (counts[i] > most.count)
            most = Count(Number(i, bits), counts[i]);
        if (counts[i] < least.count)
            least = Count(Number(i, bits), counts[i]);
    }
}

static Count mostCommon(const QBitArray &data, int bits)
{
    Count most, least;
    search(data, bits, most, least);
    return most;
}

static Count leastCommon(const QBitArray &data, int bits)
{
    Count most, least;
    search(data, bits, most, least);
    return least;
}

static QBitArray replace(const QBitArray &data, const Number &before, const Number &after)
{
    const int bits = before.bits;
    QBitArray flag(data.size());

    Number index(0, bits);
    for (int i=0; i<bits-1; i++)
        index << data.at(i);

    for (int i=0; i<data.size()-bits; i++) {
        index << data.at(i + bits - 1);
        if (index == before) {
            for (int j=0; j<bits; j++)
                flag.setBit(i+j, true);
            i += bits-1;
        }
    }

    int resultIndex = 0;
    QBitArray result(data.size());
    for (int i=0; i<data.size(); i++) {
        if (!flag.at(i)) {
            result.setBit(resultIndex, data.at(i));
            resultIndex++;
        }
    }
    result.resize(resultIndex);

    return result;
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

    uint bits = 0;
    Count least;
    while (least.count != 0) {
        bits++;
        least = leastCommon(data, bits);
    }

    qDebug("Zero value = %d (%d bits)", least.number.value, bits);

    Count most = mostCommon(data, bits);
    qDebug("Most value = %d (%d times)", most.number.value, most.count);

    QBitArray result = replace(data, most.number, least.number);
    qDebug("Size change = %d", data.size() - result.size());

    return 0;
}
