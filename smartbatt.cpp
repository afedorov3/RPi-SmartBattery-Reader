#include <cstdio>

#include "Wire.h"

const unsigned int defconf_bus = 1;
const unsigned int defconf_addr = 0x0b;
const bool defconf_force = false;

typedef enum {
  FUNC_MANUFACTURER_ACCESS = 0x00,
  FUNC_REMAINING_CAPACITY_ALARM = 0x01,
  FUNC_REMAINING_TIME_ALARM = 0x02,
  FUNC_BATTERY_MODE = 0x03,
  FUNC_AT_RATE = 0x04,
  FUNC_AT_RATE_TIME_TO_FULL = 0x05,
  FUNC_AT_RATE_TIME_TO_EMPTY = 0x06,
  FUNC_AT_RATE_OK = 0x07,
  FUNC_TEMPERATURE = 0x08,
  FUNC_VOLTAGE = 0x09,
  FUNC_CURRENT = 0x0a,
  FUNC_AVERAGE_CURRENT = 0x0b,
  FUNC_MAX_ERROR = 0x0c,
  FUNC_RELATIVE_STATE_OF_CHARGE = 0x0d,
  FUNC_ABSOLUTE_STATE_OF_CHARGE = 0x0e,
  FUNC_REMAINING_CAPACITY = 0x0f,
  FUNC_FULL_CHARGE_CAPACITY = 0x10,
  FUNC_RUN_TIME_TO_EMPTY = 0x11,
  FUNC_AVERAGE_TIME_TO_EMPTY = 0x12,
  FUNC_AVERAGE_TIME_TO_FULL = 0x13,
  FUNC_CHARGING_CURRENT = 0x14,
  FUNC_CHARGING_VOLTAGE = 0x15,
  FUNC_BATTERY_STATUS = 0x16,
  FUNC_CYCLE_COUNT = 0x17,
  FUNC_DESIGN_CAPACITY = 0x18,
  FUNC_DESIGN_VOLTAGE = 0x19,
  FUNC_SPECIFICATION_INFO = 0x1a,
  FUNC_MANUFACTURE_DATE = 0x1b,
  FUNC_SERIAL_NUMBER = 0x1c,
  // 0x1d - 0x1f reserved
  FUNC_MANUFACTURER_NAME = 0x20,
  FUNC_DEVICE_NAME = 0x21,
  FUNC_DEVICE_CHEMISTRY = 0x22,
  FUNC_MANUFACTURER_DATA = 0x23,
  // 0x25 - 0x2e reserved
  FUNC_OPTIONAL_MFG_FUNCTION5 = 0x2f,
  // 0x30 - 0x3b reserved
  FUNC_OPTIONAL_MFG_FUNCTION4 = 0x3c,
  FUNC_OPTIONAL_MFG_FUNCTION3 = 0x3d,
  FUNC_OPTIONAL_MFG_FUNCTION2 = 0x3e,
  FUNC_OPTIONAL_MFG_FUNCTION1 = 0x3f,
  FUNC_CELL4_VOLTAGE = FUNC_OPTIONAL_MFG_FUNCTION4,
  FUNC_CELL3_VOLTAGE = FUNC_OPTIONAL_MFG_FUNCTION3,
  FUNC_CELL2_VOLTAGE = FUNC_OPTIONAL_MFG_FUNCTION2,
  FUNC_CELL1_VOLTAGE = FUNC_OPTIONAL_MFG_FUNCTION1
} op_t;

enum {
  UNIT_A,
  UNIT_W
};
int cunit = UNIT_A;
int vscale = 1;
int ipscale = 1;

// fmuecke @https://stackoverflow.com/questions/1505675/power-of-an-integer-in-c#1505791
int ipow(int x, unsigned int p)
{
  if (p == 0) return 1;
  if (p == 1) return x;

  int tmp = ipow(x, p/2);
  if (p%2 == 0) return tmp * tmp;
  else return x * tmp * tmp;
}

int fetchWord(Wire &wire, op_t op, int bits, uint32_t &val)
{
  int rc;
  uint8_t buf[4];
  uint8_t *bufptr = buf;

  switch(bits) {
  case 8:
  case 16:
  case 24:
  case 32: break;
  default: return -EINVAL;
  }
  
  bits /= 8;

  rc = wire.read(op, buf, bits);
  if (rc < 0)
    return rc;

  val = 0;
  while(bits--) val = val << 8 | buf[bits];
  
  return 0;
}

int printUInt16(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;
    
  printf("%s: %u\n", tag, static_cast<uint16_t>(val));

  return 0;
}

int printXInt16(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;
    
  printf("%s: 0x%X (%d)\n", tag, static_cast<uint16_t>(val),
                                 static_cast<uint16_t>(val));

  return 0;
}

int printVoltage(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;

  printf("%s: %g V\n", tag, (double)val * vscale / 1000.0);

  return 0;
}

int printCurrent(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;

  printf("%s: %g A\n", tag, (double)static_cast<int16_t>(val) * ipscale / 1000.0);

  return 0;
}

int printCapacity(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;
    
  printf("%s: %g %s\n", tag, (double)val * ipscale / (cunit == UNIT_A ? 1.0 : 100.0),
                                                     (cunit == UNIT_A ? "mAh" : "Wh"));

  return 0;
}

int printTemperature(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;
  double tempC, tempF;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;

  tempC = (double)val / 10.0 - 273.15;
  tempF = tempC * 1.8 + 32.0;

  printf("%s: %g°C (%g°F)\n", tag, tempC, tempF);
  
  return 0;
}

int printMinutes(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;

  printf("%s: ", tag);
  if (val < 65535)
    printf("%u min (%u h %u min)\n", val, val / 60, val % 60);
  else
    puts("unknown");

  return 0;
}

int printPerc(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t val;

  rc = fetchWord(wire, op, 16, val);
  if (rc < 0)
    return rc;
    
  printf("%s: %u %%\n", tag, val);

  return 0;
}

int printString(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t cnt;
  char *buf;

  rc = fetchWord(wire, op, 8, cnt);
  if (rc < 0)
    return rc;

  ++cnt; // count
  buf = (char*)malloc(cnt);
  if (buf == NULL)
    return -errno;

  rc = wire.read(op, (uint8_t*)buf, cnt);
  if (rc < 0)
    goto free;

  printf("%s: %.*s\n", tag, cnt - 1, &buf[1]);

free:
  free(buf);

  return rc;
}

int printHex(Wire &wire, op_t op, const char *tag)
{
  int rc, i;
  uint32_t cnt;
  uint8_t *buf;

  rc = fetchWord(wire, op, 8, cnt);
  if (rc < 0)
    return rc;

  ++cnt; // count
  buf = (uint8_t*)malloc(cnt);
  if (buf == NULL)
    return -errno;

  rc = wire.read(op, buf, cnt);
  if (rc < 0)
    goto free;

  fprintf(stdout, "%s:\n", tag);
  for (i = 1; i < cnt; i++) {
    if (i % 16 == 1) fputs("  ", stdout);
    fprintf(stdout, "%02X%c", buf[i],
           (i % 16 == 0) ? '\n' : ' ');
  }
  if (i && (i % 16 != 0))
    fputc('\n', stdout);

free:
  free(buf);

  return rc;
}

int printDateCode(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t dateCode;

  rc = fetchWord(wire, op, 16, dateCode);
  if (rc < 0)
    return rc;

  int mday = dateCode & 0x1F;
  int mmonth = dateCode >> 5 & 0x0F;
  int myear = 1980 + (dateCode >> 9 & 0x7F);
  static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  printf("%s: 0x%X (%s %d, %d)\n", tag, dateCode, months[mmonth], mday, myear);

  return 0;
}

// this function sets global status vaiables
int printSpecification(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t spec;
  static const char *vers[16] = { NULL, "1.0", "1.1", "1.1 with optional PEC" };

  rc = fetchWord(wire, op, 16, spec);
  if (rc < 0)
    return rc;

  uint8_t _rev = spec & 0x0F;
  uint8_t _ver = (spec >> 4) & 0x0F;
  uint8_t _vscale = (spec >> 8) & 0x0F;
  uint8_t _ipscale = (spec >> 12) & 0x0F;

  vscale = ipow(10, _vscale);
  ipscale = ipow(10, _ipscale);

  printf("%s: Rev. %hhu, Ver. %hhu", tag, _rev, _ver);
  if (vers[_ver] != NULL)
    printf(" (%s)", vers[_ver]);
  printf(", vscale 10^%hhu = %d, ipscale 10^%hhu = %d\n",
    _vscale, vscale, _ipscale, ipscale);

  return 0;
}

// this function sets global status vaiables
int batteryMode(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t mode;
  static const char *modes0[] = {
    "Internal Charge Controller Not Supported", "Primary Battery Not Supported",
    NULL, NULL, NULL, NULL, NULL, // 2-6 reserved
    "Battery OK", "Internal Charge Control Disabled (default)", "Battery operating in its secondary role (default)",
    NULL, NULL, NULL, // 10-12 reserved
    "Broadcasts of AlarmWarning to Host and Smart Battery Charger Enabled (default)",
    "Broadcasts of ChargingVoltage and ChargingCurrent to Smart Battery Charger Enabled (default)",
    "Report in mA or mAh (default)" };
  static const char *modes1[] = {
    "Internal Charge Controller Supported", "Primary Battery Supported",
    NULL, NULL, NULL, NULL, NULL, // 2-6 reserved
    "Conditioning Cycle Requested", "Internal Charge Control Enabled", "Battery operating in its primary role",
    NULL, NULL, NULL, // 10-12 reserved
    "Broadcasts of AlarmWarning to Host and Smart Battery Charger Disabled",
    "Broadcasts of ChargingVoltage and ChargingCurrent to Smart Battery Charger Disabled",
    "Report in 10mW or 10mWh" };

  rc = fetchWord(wire, op, 16, mode);
  if (rc < 0)
    return rc;

  printf("%s: %X\n", tag, mode);

  for (int bit = 0; bit < 16; ++bit) {
    bool set = (mode & (1 << bit)) != 0;
    const char *descr = set ? modes1[bit] : modes0[bit];
    if (!descr) continue; // reserved
    printf("  Bit %d: %c %s\n", bit, set ? '1' : '0', descr);
  }

  // globals
  if (mode & (1 << 15))
    cunit = UNIT_W;
  else
    cunit = UNIT_A;

  return 0;
}

int printStatus(Wire &wire, op_t op, const char *tag)
{
  int rc;
  uint32_t status;

  rc = fetchWord(wire, op, 16, status);
  if (rc < 0)
    return rc;

  printf("%s: %X\n", tag, status);

  static const char *stat[] = { NULL, NULL, NULL, NULL, // 0-3 error codes
      "FULLY DISCHARGED", "FULLY CHARGED", "DISCHARGING", "INITIALIZED",
      "REMAINING TIME ALARM", "REMAINING CAPACITY ALARM", NULL, // 10 reserved
      "TERMINATE DISCHARGE ALARM", "OVER TEMP ALARM", NULL, // 13 reserved
      "TERMINATE CHARGE ALARM", "OVER CHARGED ALARM" };

  for (int bit = 0; bit < 16; ++bit) {
    bool set = (status & (1 << bit)) != 0;
    const char *descr = stat[bit];
    if (!set) continue; // not set
    printf("  Bit %d", bit);
    if (descr) printf(": %s", descr);
    fputc('\n', stdout);
  }

  return 0;
}

#define ARSZ(a) (sizeof((a)) / sizeof(*(a)))
int printStat(Wire &wire, bool force)
{
  typedef const struct {
    op_t op;
    int (*func)(Wire&, op_t, const char*);
    const char *tag;
  } ops_t;
  static ops_t ops[] = {
    { FUNC_SPECIFICATION_INFO, printSpecification, "Specification info" }, // must be the first
    { FUNC_BATTERY_MODE, batteryMode, "Battery mode" }, // must be the second
    { FUNC_BATTERY_STATUS, printStatus, "Battery status" },
    { FUNC_MANUFACTURER_ACCESS, printXInt16, "Manufacturer access" },
    { FUNC_MANUFACTURER_NAME, printString, "Manufacturer name" },
    { FUNC_DEVICE_NAME, printString, "Device name" },
    { FUNC_DEVICE_CHEMISTRY, printString, "Device chemistry" },
    { FUNC_SERIAL_NUMBER, printXInt16, "S/N" },
    { FUNC_MANUFACTURE_DATE, printDateCode, "Manufacture date" },
    { FUNC_MANUFACTURER_DATA, printHex, "Manufacturer data" },
    { FUNC_DESIGN_CAPACITY, printCapacity, "Design capacity" },
    { FUNC_DESIGN_VOLTAGE, printVoltage, "Design voltage" },
    { FUNC_CHARGING_VOLTAGE, printVoltage, "Charging voltage" },
    { FUNC_CHARGING_CURRENT, printCurrent, "Charging current" },
    { FUNC_TEMPERATURE, printTemperature, "Temperature" },
    { FUNC_VOLTAGE, printVoltage, "Voltage" },
    { FUNC_CELL1_VOLTAGE, printVoltage, "Cell 1 voltage" },
    { FUNC_CELL2_VOLTAGE, printVoltage, "Cell 2 voltage" },
    { FUNC_CELL3_VOLTAGE, printVoltage, "Cell 3 voltage" },
    { FUNC_CELL4_VOLTAGE, printVoltage, "Cell 4 voltage" },
    { FUNC_CURRENT, printCurrent, "Current" },
    { FUNC_AVERAGE_CURRENT, printCurrent, "Average current" },
    { FUNC_FULL_CHARGE_CAPACITY, printCapacity, "Full charge capacity" },
    { FUNC_REMAINING_CAPACITY, printCapacity, "Remaining capacity" },
    { FUNC_RELATIVE_STATE_OF_CHARGE, printPerc, "Relative state of charge" },
    { FUNC_ABSOLUTE_STATE_OF_CHARGE, printPerc, "Absolute state of charge" },
    { FUNC_AVERAGE_TIME_TO_FULL, printMinutes, "Average time to full" },
    { FUNC_AVERAGE_TIME_TO_EMPTY, printMinutes, "Average time to empty" },
    { FUNC_CYCLE_COUNT, printUInt16, "Cycle count" }
  };
  int rc = 0;
  uint32_t design_cap, full_charge_cap;

  for(int i = 0; i < ARSZ(ops); ++i) {
    ops_t *op = &ops[i];

    if (op->func == NULL) continue;

    int frc = op->func(wire, op->op, op->tag);
    if (frc < 0) {
      printf("%s failed: %s\n", op->tag, strerror(-frc));
      rc = frc;
      if (!force) return rc;
    }
  }

  rc = fetchWord(wire, FUNC_DESIGN_CAPACITY, 16, design_cap);
  if (rc == 0) {
    rc = fetchWord(wire, FUNC_FULL_CHARGE_CAPACITY, 16, full_charge_cap);
    if (rc == 0)
      printf("Maximum capacity (health): %.0f %%\n", (double)full_charge_cap / design_cap * 100.0);
  }

  return rc;
}

void usage(const char *prog)
{
  printf("usage: %s [-i i2cbus] [-a addr] [-f]\n", prog);
  printf("  -i: i2c bus number [%u]\n", defconf_bus);
  printf("  -a: SmartBattery device address [0x%02x]\n", defconf_addr);
  printf("  -f: force aquisition of all data [%s]\n", defconf_force ? "true" : "false");
}

int main(int argc, char *argv[])
{
  int rc = 0, opt;
  unsigned int bus = defconf_bus, addr = defconf_addr;
  bool force = defconf_force;
  const char *prog = basename(argv[0]);

  opterr = 0;
  while ((opt = getopt(argc, argv, "a:i:f")) != -1) {
    switch(opt) {
    case 'a': addr = strtoul(optarg, NULL, 0);
              if ((addr < 3) || (addr > 119)) {
                printf("address range is 3 .. 119\n");
                return -EINVAL;
              }
              break;
    case 'i': bus = strtoul(optarg, NULL, 0); break;
    case 'f': force = true; break;
    case '?':
    case 'h':
    default: usage(prog); return -EINVAL;
    }
  }
  argc -= optind;
  argv += optind;
  
  Wire wire(bus, addr);

  rc = printStat(wire, force);

  return rc;
}
