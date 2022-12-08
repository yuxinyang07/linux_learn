#include<linux/spi/spi.h>
#include<linux/spi/spi_gpio.h>






#define  SPI_GPIO_BUS_NUM  1
#define  SPI_GPIO_SCK   136
#define  SPI_GPIO_MISO  135 
#define  SPI_GPIO_MOSI  134
#define  SPI_GPIO_NUM_CHIPSET  133


















struct spi_gpio_platform_data  spi_gpio_info = {
    .sck = SPI_GPIO_SCK,
    .mosi = SPI_GPIO_MOSI,
    .miso = SPI_GPIO_MISO,
    .num_chipselect = SPI_GPIO_NUM_CHIPSET,  

};



struct platform_device




moudule_platform_driver();

MODULE_LICENSE("GPL");









