#include <QBitArray>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QVector>

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

    int escapeWidth = 0; //18; //0;
    int escapeValue = -1; //106484; //-1;
    int compressCount = 0;
    int compressValue = -1;
    while (escapeValue == -1) {
        escapeWidth++;
        QVector<int> occurences(1 << escapeWidth, 0);
        for (int i=0; i<data.size()-escapeWidth; i++) {
            int index = 0;
            for (int j=0; j<escapeWidth; j++)
                if (data.at(i+j)) index += 1 << j;
            occurences[index]++;
        }

        compressCount = 0;
        compressValue = -1;
        for (int i=0; i<occurences.size(); i++) {
            if (occurences[i] == 0) {
                escapeValue = i;
            } else if (occurences[i] > compressCount) {
                compressCount = occurences[i];
                compressValue = i;
            }
        }
    }

    qDebug("Escape value = %d (%d bits)", escapeValue, escapeWidth);
    qDebug("Compress value = %d (%d times)", compressValue, compressCount);

    return 0;
}
