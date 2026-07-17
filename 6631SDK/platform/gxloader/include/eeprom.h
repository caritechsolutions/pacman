#ifndef __EEPROM_H__
#define __EEPROM_H__

int eeprom_init(unsigned int bus_id, unsigned char chip_addr, char *eeprom_name);
ssize_t eeprom_write(unsigned char *buf, unsigned int off, size_t count);
ssize_t eeprom_read(unsigned char *buf, unsigned int off, size_t count);

#endif /* __EEPROM_H__*/
