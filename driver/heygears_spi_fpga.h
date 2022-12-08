#ifndef __HEYGEARS_SPI_FPGA_H__
#define __HEYGEARS_SPI_FPGA_H__


#define SPI_GPIO_NO_NCS         ((unsigned long)-1l)
#define SPI_GPIO_NO_MISO        ((unsigned long)-1l)
#define SPI_GPIO_NO_MOSI        ((unsigned long)-1l)
#define SPI_GPIO_NO_SCK         ((unsigned long)-1l)


#define SPI_CS_SELECT           0
#define SPI_CS_NOSELECT         1


#define LSC_READ_IDCODE         0xE0    
#define LSC_READ_STATUS         0x3C
#define LSC_INIT_ADDR           0x46    
#define LSC_ENABLE_X            0x74
#define LSC_BITSTREAM_BURST     0x7A
#define LSC_READ_USERCODE       0xC0

#define ISC_ERASE               0x0E
#define ISC_DISABLE             0x26
#define ISC_ENABLE              0xC6


struct spi_fpga_gpio {
    unsigned long   sck;
    unsigned long   mosi;
    unsigned long   miso;
    unsigned long   ncs;
    
    unsigned long   prog;
    unsigned long   init;
    unsigned long   done;
    
    unsigned long   power;
};


#endif	/* __HEYGEARS_SPI_FPGA_H__ */
