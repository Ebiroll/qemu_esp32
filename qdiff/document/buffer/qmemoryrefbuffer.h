#ifndef QMEMORYREFBUFFER_H
#define QMEMORYREFBUFFER_H

#include "qhexbuffer.h"
#include "qbuffer.h"

class QMemoryRefBuffer : public QHexBuffer
{
    Q_OBJECT

    public:
        explicit QMemoryRefBuffer(QObject *parent = NULL);
        virtual int length() const;
        virtual void insert(int, const QByteArray&);
        virtual void remove(int offset, int length);
        virtual QByteArray read(int offset, int length);
        virtual void read(QIODevice* device);
        virtual void write(QIODevice* device);

    private:
        QBuffer* m_buffer;
};

#endif // QMEMORYREFBUFFER_H
