 #include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

// 可以读的数据的大小 W指针-R指针，中间的数据就是可以读的大小
size_t Buffer::ReadableBytes() const {  
    return writePos_ - readPos_;
}

// buffer_中目前可以写的数据大小，缓冲区的总大小-写位置
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 前面可以利用的空间，当前读取到哪个位置，就是前面可以用的空间大小，因为已经读到了，就不需要前面的空间了
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

//buff.RetrieveUntil(lineEnd + 2);
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek());
}

//初始化缓冲区：清零，读写位置复位
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size()); //内存归0
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len;
} 

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

/*  Append(buff, len - writable);   buff临时数组，len-writable是临时数组中的数据个数
 将临时数组里的数据追加到buffer_中，其中如果不够需要进行memmove或者扩容后memmove
 */
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::EnsureWriteable(size_t len) {
    //如果buffer_可写的字节数 小于 多出来的这个len，就扩容
    if(WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

//具体执行 将http请求数据从cfd中读到自己vector中 的操作，返回读到的字节数
ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    
    char buff[65535];   // 临时的数组，保证能够把所有的数据都读出来
    
    /*创建一个IO向量数组
     struct iovec {
         iov_base; 指向目前buffer_的首地址
         iov_len;  记录目前buffer_的有效长度
     }
     */
    struct iovec iov[2];  //第一个向量是原vectorbuffer，第二个向量是临时数组
    const size_t writable = WritableBytes(); //缓冲区可写数据大小
    
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = BeginPtr_() + writePos_;  //起始地址偏移写指针个量
    iov[0].iov_len = writable;  //可写数据大小就是有效长度
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);  //分散读，先紧着iov[0]读满再读到下一个
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {  //如果读到的字节数<=目前vector可写容量
        writePos_ += len;  //那就说明不用扩容，写指针移动len即可
    }
    else {       //如果读到的字节数len > 目前vector可写容量了，就说明临时数组里也存了一些
        writePos_ = buffer_.size();   //1.首先写指针移动到末尾
        Append(buff, len - writable); //2.其次将临时数组中的数据接到后面
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    readPos_ += len;
    return len;
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::MakeSpace_(size_t len) {
    //如果目前可读+前面已读<len，说明vector不够用，需扩容 
    if(WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    }
    //如果够用，就移动 
    else {
        size_t readable = ReadableBytes();  //就是W指针-R指针，就是目前buffer_里有的数据长度
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}