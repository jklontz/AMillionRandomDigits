#include <QBitArray>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QVector>

struct Count
{
    int value, count;
    Count() : value(-1), count(-1) {}
    Count(int _value, int _count) : value(_value), count(_count) {}
};

static void search(const QBitArray &data, int bits, Count &most, Count &least)
{
    QVector<int> occurences(1 << bits, 0);
    const int mask = (1 << bits) - 1;

    int index = 0;
    for (int i=0; i<bits-1; i++)
        if (data.at(i)) index += 1 << (bits-i-1);

    for (int i=0; i<data.size()-bits; i++) {
        index = ((index << 1) & mask) + (data.at(i + bits - 1) ? 1 : 0);
        occurences[index]++;
    }

    int mostCommonValue = -1;
    int mostCommonCount = 0;
    int leastCommonValue = -1;
    int leastCommonCount = data.size();
    for (int i=0; i<occurences.size(); i++) {
        if (occurences[i] > mostCommonCount) {
            mostCommonCount = occurences[i];
            mostCommonValue = i;
        }
        if (occurences[i] < leastCommonCount) {
            leastCommonCount = occurences[i];
            leastCommonValue = i;
        }
    }

    most = Count(mostCommonValue, mostCommonCount);
    least = Count(leastCommonValue, leastCommonCount);
}

Count mostCommon(const QBitArray &data, int bits)
{
    Count most, least;
    search(data, bits, most, least);
    return most;
}

Count leastCommon(const QBitArray &data, int bits)
{
    Count most, least;
    search(data, bits, most, least);
    return least;
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

    int bits = 0;
    Count least;
    while (least.count != 0) {
        bits++;
        least = leastCommon(data, bits);
    }

    qDebug("Zero value = %d (%d bits)", least.value, bits);

    Count most = mostCommon(data, bits);
    qDebug("Most value = %d (%d times)", most.value, most.count);

    return 0;
}
