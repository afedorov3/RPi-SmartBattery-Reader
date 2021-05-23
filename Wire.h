#ifndef _WIRE_H
#define _WIRE_H

#include <unistd.h> // read, write, close
#include <cstdint>
#include <cstring> // memcpy
#include <cerrno>
#include <string>
#include <linux/i2c-dev.h>
#ifndef I2C_M_RD
#include <linux/i2c.h>
#endif
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef DBG
#include <ctime.h>
#endif

class Wire
{
public:
  Wire(uint8_t bus):
    _flags(0),
    _addr(0),
    _regb(1) {
    setBus(bus);
  }

  Wire(uint8_t bus, uint16_t addr):
    Wire(bus) {
    setAddr(addr);
  }

  Wire(uint8_t bus, uint16_t addr, uint8_t width):
    Wire(bus) {
    setAddr(addr, width);
  }

  void setBus(uint8_t bus) {
    _busdev = "/dev/i2c-" + std::to_string(bus);
  }

  void setAddr(uint16_t addr) {
    _addr = addr;
    if (_addr > 0xFF)
      _flags |= I2C_M_TEN;
  }

  void setAddr(uint16_t addr, uint8_t width) {
    setAddr(addr);
// TODO
//    _regb = width / 8;
  }

  int write(int32_t reg, uint8_t buf[], uint16_t count) {
    int rc = 0;
    uint8_t tbuf[count + _regb];
    uint32_t tcount = count + _regb;
    uint16_t addr;

    if (_addr < 3)
      return -ENXIO;
    if (reg < 0)
      return -EINVAL;

#ifdef DBG
    printf("going to WRITE to %d from %p, count %d\n", reg, buf, count);
    struct timespec tm, tm2;
    clock_gettime(CLOCK_REALTIME, &tm);
#endif

    int file = ::open(_busdev.c_str(), O_RDWR);
    if (file == -1) {
      return -errno;
    }

// TODO: reg addr
    addr = _addr;
    tbuf[0] = reg;
//
    if (ioctl(file, I2C_SLAVE, addr) == -1) {
      rc = -errno;
      goto writeclose;
    }

    memcpy(&tbuf[_regb], buf, count);
    if (::write(file, tbuf, tcount) != tcount) {
      rc = -errno;
      goto writeclose;
    }

#ifdef DBG
    clock_gettime(CLOCK_REALTIME, &tm2);
    fprintf(stderr, "write complete in %.1fus\n", 
      ((tm2.tv_nsec - tm.tv_nsec) / 100 ) / 10.0);
#endif

writeclose:
    ::close(file);

    return rc;
  }

  int read(int32_t reg, uint8_t buf[], uint16_t count) {
    int rc = 0;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data iodata;
    uint8_t rbuf[_regb];
    uint16_t addr;
    uint8_t p = 0;

    if (_addr < 3)
      return -ENXIO;

#ifdef DBG
    printf("going to READ from %d to %p, count %d\n", reg, buf, count);
    struct timespec tm, tm2;
    clock_gettime(CLOCK_REALTIME, &tm);
#endif

    int file = ::open(_busdev.c_str(), O_RDWR);
    if (file == -1) {
      return -errno;
    }

    if (reg >= 0) {
// TODO: reg addr
      addr = _addr;
      rbuf[0] = reg;
//
      msgs[p].addr = addr;
      msgs[p].flags = _flags;
      msgs[p].len = _regb;
      msgs[p].buf = (typeof i2c_msg::buf)rbuf;
      p++;
    }
    msgs[p].addr = _addr;
    msgs[p].flags = _flags | I2C_M_RD;
    msgs[p].len = count;
    msgs[p].buf = (typeof i2c_msg::buf)buf;
    p++;
    iodata.msgs = msgs;
    iodata.nmsgs = p;
    if (ioctl(file, I2C_RDWR, &iodata) != p) {
      rc = -errno;
      goto readclose;
    }

#ifdef DBG
    clock_gettime(CLOCK_REALTIME, &tm2);
    fprintf(stderr, "read complete in %.1fus\n",
      ((tm2.tv_nsec - tm.tv_nsec) / 100 ) / 10.0);
#endif

readclose:
    ::close(file);

    return rc;
  }

private:
  std::string _busdev;
  uint16_t _addr;
  uint8_t _regb;
  uint16_t _flags;
};

#endif // _WIRE_H
