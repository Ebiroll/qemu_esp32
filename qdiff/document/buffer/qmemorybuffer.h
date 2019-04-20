#ifndef QMEMORYBUFFER_H
#define QMEMORYBUFFER_H

#include "qhexbuffer.h"

class QMemoryBuffer : public QHexBuffer
{
    Q_OBJECT

    public:
        explicit QMemoryBuffer(QObject *parent = NULL);
        virtual uchar at(int idx);
        virtual int length() const;
        virtual void insert(int offset, const QByteArray& data);
        virtual void remove(int offset, int length);
        virtual QByteArray read(int offset, int length);
        virtual void read(QIODevice* device);
        virtual void write(QIODevice* device);

    private:
        QByteArray m_buffer;
};

#endif // QMEMORYBUFFER_H
