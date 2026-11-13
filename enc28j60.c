#include "enc28j60.h"
#include "stm32f1xx_hal.h"
extern SPI_HandleTypeDef hspi1;
static uint16_t enc_next_packet = ENC28J60_RX_BUF_START;
/* Кэш банка действует только при эксклюзивном доступе к ENC28J60 по SPI. */
static uint8_t enc_current_bank = 0xFF;
/* 1. Низкоуровневые функции SPI */
static void ENC28J60_SetCS(GPIO_PinState state)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, state);
}
static HAL_StatusTypeDef ENC28J60_BitFieldSetRaw(uint8_t regAddr,
                                                 uint8_t mask)
{
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFS | (regAddr & 0x1F)), mask};
    HAL_StatusTypeDef status;
    ENC28J60_SetCS(GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);
    ENC28J60_SetCS(GPIO_PIN_SET);
    return status;
}
static HAL_StatusTypeDef ENC28J60_BitFieldClearRaw(uint8_t regAddr,
                                                   uint8_t mask)
{
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_BFC | (regAddr & 0x1F)), mask};
    HAL_StatusTypeDef status;
    ENC28J60_SetCS(GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, tx, 2, ENC28J60_SPI_TIMEOUT_MS);
    ENC28J60_SetCS(GPIO_PIN_SET);
    return status;
}
/* Переключение банков только при необходимости. */
static HAL_StatusTypeDef ENC28J60_SelectBank(uint8_t bank)
{
    HAL_StatusTypeDef status;
    bank &= 0x03;
    if (enc_current_bank == bank) return HAL_OK;

    status = ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_BSEL0 | ENC28J60_ECON1_BSEL1);
    if (status != HAL_OK)
    {
        enc_current_bank = 0xFF;
        return status;
    }
    status = ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1, bank);
    enc_current_bank = (status == HAL_OK) ? bank : 0xFF;
    return status;
}
/* Чтение регистра с отдельным статусом SPI; нулевое значение не означает ошибку. */
static HAL_StatusTypeDef ENC28J60_ReadControlReg(uint8_t reg,
                                                uint8_t *value)
{
    uint8_t addr = reg & 0x1F;
    uint8_t bank = (reg >> 5) & 0x03;
    uint8_t isMacMii = (reg & 0x80) != 0;
    uint8_t tx[3] = {addr, 0x00, 0x00};
    uint8_t rx[3] = {0};
    uint16_t size = isMacMii ? 3 : 2;
    HAL_StatusTypeDef status;

    if (value == 0) return HAL_ERROR;
    *value = 0;

    /* Общие ETH-регистры не требуют выбора банка. */
    if (addr < 0x1B)
    {
        status = ENC28J60_SelectBank(bank);
        if (status != HAL_OK) return status;
    }

    ENC28J60_SetCS(GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, size,
                                     ENC28J60_SPI_TIMEOUT_MS);
    ENC28J60_SetCS(GPIO_PIN_SET);
    if (status == HAL_OK) *value = rx[size - 1];
    return status;
}
static HAL_StatusTypeDef ENC28J60_WriteControlReg(uint8_t reg,
                                                  uint8_t value)
{
    uint8_t addr = reg & 0x1F;
    uint8_t bank = (reg >> 5) & 0x03;
    uint8_t tx[2] = {(uint8_t)(ENC28J60_CMD_WCR | addr), value};
    HAL_StatusTypeDef status;
    if (addr < 0x1B)
    {
        status = ENC28J60_SelectBank(bank);
        if (status != HAL_OK)
        {
            return status;
        }
    }
    ENC28J60_SetCS(GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, tx, 2,
                              ENC28J60_SPI_TIMEOUT_MS);
    ENC28J60_SetCS(GPIO_PIN_SET);
    return status;
}
static HAL_StatusTypeDef ENC28J60_WriteControlRegPair(uint8_t reg,
                                                      uint16_t value)
{
    HAL_StatusTypeDef status;
    status = ENC28J60_WriteControlReg(reg, (uint8_t)value);
    if (status != HAL_OK)
    {
        return status;
    }
    status = ENC28J60_WriteControlReg((uint8_t)(reg + 1),
                                     (uint8_t)(value >> 8));
    return status;
}
/* Чтение пары регистров с контролем каждой транзакции SPI. */
static HAL_StatusTypeDef ENC28J60_ReadControlRegPair(uint8_t reg,
                                                    uint16_t *value)
{
    uint8_t low, high;
    HAL_StatusTypeDef status;

    if (value == 0) return HAL_ERROR;
    *value = 0;
    status = ENC28J60_ReadControlReg(reg, &low);
    if (status != HAL_OK) return status;
    status = ENC28J60_ReadControlReg((uint8_t)(reg + 1), &high);
    if (status != HAL_OK) return status;
    *value = (uint16_t)((uint16_t)low | ((uint16_t)high << 8));
    return HAL_OK;
}
static HAL_StatusTypeDef ENC28J60_WriteBufferMem(uint8_t *data,
                                                 uint16_t size)
{
    uint8_t command = ENC28J60_CMD_WBM;
    HAL_StatusTypeDef status;
    if (data == 0 || size == 0)
    {
        return HAL_ERROR;
    }
    ENC28J60_SetCS(GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, &command, 1,
                              ENC28J60_SPI_TIMEOUT_MS);
    if (status == HAL_OK)
    {
        status = HAL_SPI_Transmit(&hspi1, data, size,
                                  ENC28J60_SPI_TIMEOUT_MS);
    }
    ENC28J60_SetCS(GPIO_PIN_SET);
    return status;
}
static HAL_StatusTypeDef ENC28J60_ReadBufferMem(uint8_t *data,
                                                uint16_t size)
{
    uint8_t command = ENC28J60_CMD_RBM;
    HAL_StatusTypeDef status;
    if (data == 0 || size == 0)
    {
        return HAL_ERROR;
    }
    ENC28J60_SetCS(GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, &command, 1,
                              ENC28J60_SPI_TIMEOUT_MS);
    if (status == HAL_OK)
    {
        status = HAL_SPI_Receive(&hspi1, data, size,
                                 ENC28J60_SPI_TIMEOUT_MS);
    }
    ENC28J60_SetCS(GPIO_PIN_SET);
    return status;
}
/* 2. Сброс и настройка ENC28J60 */
uint8_t ENC28J60_SoftReset(void)
{
    uint8_t command = ENC28J60_CMD_SRC;
    HAL_StatusTypeDef status;
    ENC28J60_SetCS(GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(&hspi1, &command, 1,
                              ENC28J60_SPI_TIMEOUT_MS);
    ENC28J60_SetCS(GPIO_PIN_SET);
    if (status != HAL_OK)
    {
        return 0;
    }
    enc_current_bank = 0xFF;
    HAL_Delay(ENC28J60_RESET_DELAY_MS);
    return 1;
}
uint8_t ENC28J60_ReadRevision(void)
{
    uint8_t value = 0;
    /* 0 - недопустимая ревизия; используется также при ошибке SPI. */
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EREVID, &value) != HAL_OK)
        return 0;
    return value;
}
/* Настройка кольцевого RX-буфера. Граница ERXRDPT всегда нечётная. */
uint8_t ENC28J60_ConfigureBuffers(void)
{
    uint16_t check;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXSTL,
                                    ENC28J60_RX_BUF_START) != HAL_OK) return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXNDL,
                                    ENC28J60_RX_BUF_END) != HAL_OK) return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXRDPTL,
                                    ENC28J60_RX_BUF_END) != HAL_OK) return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,
                                    ENC28J60_RX_BUF_START) != HAL_OK) return 0;

    /* RBM/WBM используют автоматическое увеличение адреса RAM. */
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,
                               ENC28J60_ECON2_AUTOINC) != HAL_OK) return 0;

    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXSTL, &check) != HAL_OK ||
        check != ENC28J60_RX_BUF_START) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXNDL, &check) != HAL_OK ||
        check != ENC28J60_RX_BUF_END) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERXRDPTL, &check) != HAL_OK ||
        check != ENC28J60_RX_BUF_END) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ERDPTL, &check) != HAL_OK ||
        check != ENC28J60_RX_BUF_START) return 0;
    enc_next_packet = ENC28J60_RX_BUF_START;
    return 1;
}
/* Согласованная настройка MAC для выбранного Duplex. */
uint8_t ENC28J60_ConfigureMAC(ENC28J60_DuplexMode duplex)
{
    uint8_t reg_value;
    uint16_t check;
    uint8_t macon3_value = ENC28J60_MACON3_VALUE;
    uint8_t mabbipg_value = ENC28J60_MABBIPG_VALUE;
    uint16_t maipg_value = ENC28J60_MAIPG_VALUE;

    if (duplex != ENC28J60_DUPLEX_HALF &&
        duplex != ENC28J60_DUPLEX_FULL) return 0;

    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &reg_value) != HAL_OK)
        return 0;
    if (reg_value & (ENC28J60_ECON1_RXEN | ENC28J60_ECON1_TXRTS))
        return 0;

    if (duplex == ENC28J60_DUPLEX_FULL)
    {
        macon3_value |= ENC28J60_MACON3_FULDPX;
        mabbipg_value = ENC28J60_MABBIPG_FULL;
        maipg_value = ENC28J60_MAIPG_FULL;
    }
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON1,
                                 ENC28J60_MACON1_VALUE) != HAL_OK) return 0;
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON3,
                                 macon3_value) != HAL_OK) return 0;
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MABBIPG,
                                 mabbipg_value) != HAL_OK) return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MAIPGL,
                                     maipg_value) != HAL_OK) return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MAMXFLL,
                                     ENC28J60_MAMXFL_VALUE) != HAL_OK) return 0;

    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON1, &reg_value) != HAL_OK ||
        reg_value != ENC28J60_MACON1_VALUE) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON3, &reg_value) != HAL_OK ||
        reg_value != macon3_value) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MABBIPG, &reg_value) != HAL_OK ||
        reg_value != mabbipg_value) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAIPGL, &check) != HAL_OK ||
        check != maipg_value) return 0;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_MAMXFLL, &check) != HAL_OK ||
        check != ENC28J60_MAMXFL_VALUE) return 0;
    return 1;
}
/* Программирование MAC-адреса из конфигурации приложения. */
uint8_t ENC28J60_ConfigureMACAddress(const uint8_t *mac)
{
    const uint8_t regs[ENC28J60_MAC_ADDRESS_SIZE] =
    {
        ENC28J60_REG_MAADR1, ENC28J60_REG_MAADR2,
        ENC28J60_REG_MAADR3, ENC28J60_REG_MAADR4,
        ENC28J60_REG_MAADR5, ENC28J60_REG_MAADR6
    };
    uint8_t i, value;
    if (mac == 0) return 0;
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)
    {
        if (ENC28J60_WriteControlReg(regs[i], mac[i]) != HAL_OK)
            return 0;
    }
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)
    {
        if (ENC28J60_ReadControlReg(regs[i], &value) != HAL_OK ||
            value != mac[i]) return 0;
    }
    return 1;
}
/* 3. Работа с PHY */
/* MII-доступ с тайм-аутом, без блокирующего HAL_Delay(1). */
static HAL_StatusTypeDef ENC28J60_WaitMII(void)
{
    uint32_t startTime = HAL_GetTick();
    uint8_t state;
    do
    {
        if (ENC28J60_ReadControlReg(ENC28J60_REG_MISTAT, &state) != HAL_OK)
            return HAL_ERROR;
        if ((state & ENC28J60_MISTAT_BUSY) == 0)
            return HAL_OK;
    }
    while ((uint32_t)(HAL_GetTick() - startTime) < ENC28J60_PHY_TIMEOUT_MS);
    return HAL_TIMEOUT;
}

uint8_t ENC28J60_ReadPHY(uint8_t reg, uint16_t *value)
{
    HAL_StatusTypeDef status;
    if (value == 0) return 0;
    *value = 0;
    status = ENC28J60_WriteControlReg(ENC28J60_REG_MIREGADR, reg);
    if (status != HAL_OK) return 0;
    status = ENC28J60_WriteControlReg(ENC28J60_REG_MICMD,
                                     ENC28J60_MICMD_MIIRD);
    if (status != HAL_OK) return 0;

    status = ENC28J60_WaitMII();
    /* Бит MIIRD обязательно снимается даже после тайм-аута. */
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MICMD, 0) != HAL_OK)
        return 0;
    if (status != HAL_OK) return 0;
    return (ENC28J60_ReadControlRegPair(ENC28J60_REG_MIRDL, value) == HAL_OK);
}

uint8_t ENC28J60_WritePHY(uint8_t reg, uint16_t value)
{
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MIREGADR, reg) != HAL_OK)
        return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_MIWRL, value) != HAL_OK)
        return 0;
    return (ENC28J60_WaitMII() == HAL_OK);
}
uint8_t ENC28J60_StartReceiving(void)
{
    uint8_t value;
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,
                                 ENC28J60_ECON1_RXEN) != HAL_OK) return 0;
    /* DEFER не воздействует на Full Duplex. */
    if (ENC28J60_WriteControlReg(ENC28J60_REG_MACON4,
                                 ENC28J60_MACON4_VALUE) != HAL_OK) return 0;
    if (ENC28J60_WriteControlReg(ENC28J60_REG_ERXFCON,
                                 ENC28J60_ERXFCON_VALUE) != HAL_OK) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_MACON4, &value) != HAL_OK ||
        value != ENC28J60_MACON4_VALUE) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ERXFCON, &value) != HAL_OK ||
        value != ENC28J60_ERXFCON_VALUE) return 0;
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,
                               ENC28J60_ECON1_RXEN) != HAL_OK) return 0;
    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &value) != HAL_OK)
        return 0;
    return ((value & ENC28J60_ECON1_RXEN) != 0);
}
ENC28J60_InitResult ENC28J60_Init(const uint8_t *mac,
                                   ENC28J60_DuplexMode duplex)
{
    uint8_t revision;
    uint16_t phy_value = 0;
    uint16_t phy_duplex;
    uint16_t phy_loopback;
    uint8_t i;
    uint8_t mac_nonzero = 0;
    if (mac == 0 ||
        (duplex != ENC28J60_DUPLEX_HALF &&
         duplex != ENC28J60_DUPLEX_FULL))
    {
        return ENC28J60_INIT_ERR_PARAM;
    }
    if ((mac[0] & 0x01) != 0)
    {
        return ENC28J60_INIT_ERR_PARAM;
    }
    for (i = 0; i < ENC28J60_MAC_ADDRESS_SIZE; i++)
    {
        if (mac[i] != 0)
        {
            mac_nonzero = 1;
        }
    }
    if (mac_nonzero == 0)
    {
        return ENC28J60_INIT_ERR_PARAM;
    }
    phy_duplex = (duplex == ENC28J60_DUPLEX_FULL)
               ? ENC28J60_PHCON1_PDPXMD : 0;
    phy_loopback = (duplex == ENC28J60_DUPLEX_HALF)
                 ? ENC28J60_PHCON2_VALUE : 0;
    if (ENC28J60_SoftReset() != 1)
    {
        return ENC28J60_INIT_ERR_RESET;
    }
    revision = ENC28J60_ReadRevision();
    if (revision == 0x00 || revision == 0xFF)
    {
        return ENC28J60_INIT_ERR_REVISION;
    }
    if (ENC28J60_ConfigureBuffers() != 1)
    {
        return ENC28J60_INIT_ERR_BUFFERS;
    }
    if (ENC28J60_ConfigureMAC(duplex) != 1)
    {
        return ENC28J60_INIT_ERR_MAC;
    }
    if (ENC28J60_ConfigureMACAddress(mac) != 1)
    {
        return ENC28J60_INIT_ERR_ADDRESS;
    }
    if (ENC28J60_WritePHY(
        ENC28J60_PHY_PHCON1,
        phy_duplex) != 1)
    {
        return ENC28J60_INIT_ERR_PHY;
    }
    if (ENC28J60_ReadPHY(
        ENC28J60_PHY_PHCON1,
        &phy_value) != 1 ||
        (phy_value & ENC28J60_PHCON1_PDPXMD) != phy_duplex)
    {
        return ENC28J60_INIT_ERR_PHY;
    }
    if (ENC28J60_WritePHY(
        ENC28J60_PHY_PHCON2,
        phy_loopback) != 1)
    {
        return ENC28J60_INIT_ERR_PHY;
    }
    if (ENC28J60_ReadPHY(
        ENC28J60_PHY_PHCON2,
        &phy_value) != 1 ||
        (phy_value & ENC28J60_PHCON2_HDLDIS) != phy_loopback)
    {
        return ENC28J60_INIT_ERR_PHY;
    }
    if (ENC28J60_StartReceiving() != 1)
    {
        return ENC28J60_INIT_ERR_RX;
    }
    return ENC28J60_INIT_OK;
}
/* 4. Работа с Ethernet-кадрами */
uint8_t ENC28J60_GetPacketCount(void)
{
    uint8_t count = 0;
    /* Исторический интерфейс возвращает 0 также при ошибке SPI. */
    if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &count) != HAL_OK)
        return 0;
    return count;
}
static uint8_t ENC28J60_PrepareTxFrame(uint8_t *data,
                                      uint16_t size)
{
    uint8_t controlByte = 0x00, econ1;
    if (data == 0 || size < ENC28J60_FRAME_MIN ||
        size > ENC28J60_TX_FRAME_MAX) return 0;

    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &econ1) != HAL_OK)
        return 0;
    if (econ1 & ENC28J60_ECON1_TXRTS) return 0;

    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_EWRPTL,
                                     ENC28J60_TX_BUF_START) != HAL_OK) return 0;
    /* Управляющий байт на один адрес раньше данных кадра. */
    if (ENC28J60_WriteBufferMem(&controlByte, 1) != HAL_OK) return 0;
    if (ENC28J60_WriteBufferMem(data, size) != HAL_OK) return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ETXSTL,
                                     ENC28J60_TX_BUF_START) != HAL_OK) return 0;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ETXNDL,
                                     (uint16_t)(ENC28J60_TX_BUF_START + size))
        != HAL_OK) return 0;
    return 1;
}
static uint8_t ENC28J60_ResetTxLogic(void)
{
    if (ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRTS) != HAL_OK)
    {
        return 0;
    }
    if (ENC28J60_BitFieldSetRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRST) != HAL_OK)
    {
        return 0;
    }
    if (ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_ECON1,
        ENC28J60_ECON1_TXRST) != HAL_OK)
    {
        return 0;
    }
    if (ENC28J60_BitFieldClearRaw(
        ENC28J60_REG_EIR,
        ENC28J60_EIR_TXERIF | ENC28J60_EIR_TXIF) != HAL_OK)
    {
        return 0;
    }
    return 1;
}
/* Чтение семибайтового TSV. Адрес: ETXND + 1. */
static HAL_StatusTypeDef ENC28J60_ReadTxStatus(uint8_t status[7])
{
    uint16_t end;
    if (ENC28J60_ReadControlRegPair(ENC28J60_REG_ETXNDL, &end) != HAL_OK)
        return HAL_ERROR;
    /* Семь байт TSV должны полностью находиться в TX RAM. */
    if (end > (ENC28J60_TX_BUF_END - 7)) return HAL_ERROR;
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,
                                     (uint16_t)(end + 1)) != HAL_OK)
        return HAL_ERROR;
    return ENC28J60_ReadBufferMem(status, 7);
}

/* При поздней коллизии B7 повторяем TX, максимум 3 раза сверх первой попытки. */
#define ENC28J60_TX_LATE_RETRIES 3u
#define ENC28J60_TX_TSV_LATECOL 0x20u  /* TSV: bit 29 = byte 3, bit 5 */
ENC28J60_TxResult ENC28J60_TransmitFrame(uint8_t *data,
                                         uint16_t size)
{
    uint16_t phy_status = 0;
    uint32_t startTime;
    uint8_t flags, estat, econ1, attempts;
    uint8_t tsv[7];
    uint8_t late_collision;

    if (data == 0 || size < ENC28J60_FRAME_MIN ||
        size > ENC28J60_TX_FRAME_MAX)
        return ENC28J60_TX_INVALID_FRAME;

    if (ENC28J60_ReadControlReg(ENC28J60_REG_ECON1, &econ1) != HAL_OK)
        return ENC28J60_TX_ERROR;
    if (econ1 & ENC28J60_ECON1_TXRTS) return ENC28J60_TX_BUSY;

    if (ENC28J60_ReadPHY(ENC28J60_PHY_PHSTAT2, &phy_status) != 1)
        return ENC28J60_TX_ERROR;
    if ((phy_status & ENC28J60_PHSTAT2_LSTAT) == 0)
        return ENC28J60_TX_NO_LINK;

    if (ENC28J60_ResetTxLogic() != 1) return ENC28J60_TX_ERROR;
    if (ENC28J60_PrepareTxFrame(data, size) != 1)
        return ENC28J60_TX_ERROR;

    for (attempts = 0; attempts <= ENC28J60_TX_LATE_RETRIES; attempts++)
    {
        /* При повторах TX-данные остаются в памяти ENC28J60. */
        if (attempts > 0 && ENC28J60_ResetTxLogic() != 1)
            return ENC28J60_TX_ERROR;
        if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,
                                   ENC28J60_ECON1_TXRTS) != HAL_OK)
            return ENC28J60_TX_ERROR;

        startTime = HAL_GetTick();
        for (;;)
        {
            if (ENC28J60_ReadControlReg(ENC28J60_REG_EIR, &flags) != HAL_OK)
            {
                ENC28J60_ResetTxLogic();
                return ENC28J60_TX_ERROR;
            }
            if (flags & (ENC28J60_EIR_TXERIF | ENC28J60_EIR_TXIF))
                break;
            if ((uint32_t)(HAL_GetTick() - startTime) >= ENC28J60_TX_TIMEOUT_MS)
            {
                ENC28J60_ResetTxLogic();
                return ENC28J60_TX_TIMEOUT;
            }
        }

        if (ENC28J60_ReadControlReg(ENC28J60_REG_ESTAT, &estat) != HAL_OK)
        {
            ENC28J60_ResetTxLogic();
            return ENC28J60_TX_ERROR;
        }
        /* TXIF сам по себе не означает успех; TXABRT имеет приоритет. */
        if ((flags & ENC28J60_EIR_TXERIF) == 0 &&
            (flags & ENC28J60_EIR_TXIF) != 0 &&
            (estat & ENC28J60_ESTAT_TXABRT) == 0)
            return ENC28J60_TX_OK;

        /* Errata B7: при Late Collision TXRTS может остаться установленным.
         * Завершаем неудачную попытку перед чтением TSV и новым запуском TX.
         * Успешная передача уже обработана выше, поэтому кадр здесь не теряем.
         */
        if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,
                                     ENC28J60_ECON1_TXRTS) != HAL_OK)
        {
            ENC28J60_ResetTxLogic();
            return ENC28J60_TX_ERROR;
        }

        late_collision = 0;
        if (ENC28J60_ReadTxStatus(tsv) != HAL_OK)
        {
            ENC28J60_ResetTxLogic();
            return ENC28J60_TX_ERROR;
        }
        if ((tsv[3] & ENC28J60_TX_TSV_LATECOL) != 0)
            late_collision = 1;
        if (!late_collision || attempts == ENC28J60_TX_LATE_RETRIES)
        {
            ENC28J60_ResetTxLogic();
            return ENC28J60_TX_ERROR;
        }
    }
    return ENC28J60_TX_ERROR;
}
/* При повреждении Next Packet Pointer безопасно переинициализируем RX. */
static uint8_t ENC28J60_RecoverRx(void)
{
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,
                                 ENC28J60_ECON1_RXEN) != HAL_OK) return 0;
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON1,
                               ENC28J60_ECON1_RXRST) != HAL_OK) return 0;
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_ECON1,
                                 ENC28J60_ECON1_RXRST) != HAL_OK) return 0;
    if (ENC28J60_BitFieldClearRaw(ENC28J60_REG_EIR,
                                 ENC28J60_EIR_RXERIF) != HAL_OK) return 0;

    /* RXRST не гарантирует очистку EPKTCNT: освобождаем счётчик явно. */
    {
        uint8_t pending = 0;
        if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &pending) != HAL_OK)
            return 0;
        while (pending-- != 0)
        {
            if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,
                                       ENC28J60_ECON2_PKTDEC) != HAL_OK)
                return 0;
        }
        /* Если счетчик остался ненулевым, RX считается невосстановленным. */
        if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &pending) != HAL_OK ||
            pending != 0)
            return 0;
    }
    if (ENC28J60_ConfigureBuffers() != 1) return 0;
    return ENC28J60_StartReceiving();
}

ENC28J60_RxResult ENC28J60_ReceiveFrame(uint8_t *data,
                                        uint16_t capacity,
                                        uint16_t *receivedLen)
{
    uint8_t header[ENC28J60_RX_HEADER_SIZE] = {0};
    uint8_t packetCount = 0;
    uint16_t nextPacket, frameSize, rxStatus, dataSize, freePtr;
    ENC28J60_RxResult result = ENC28J60_RX_BAD_FRAME;

    if (data == 0 || receivedLen == 0 || capacity == 0)
        return ENC28J60_RX_INVALID_ARG;
    *receivedLen = 0;

    if (ENC28J60_ReadControlReg(ENC28J60_REG_EPKTCNT, &packetCount)
        != HAL_OK) return ENC28J60_RX_SPI_ERROR;
    if (packetCount == 0) return ENC28J60_RX_EMPTY;

    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERDPTL,
                                     enc_next_packet) != HAL_OK)
        return ENC28J60_RX_SPI_ERROR;
    if (ENC28J60_ReadBufferMem(header, ENC28J60_RX_HEADER_SIZE) != HAL_OK)
        return ENC28J60_RX_SPI_ERROR;

    nextPacket = (uint16_t)(header[0] | ((uint16_t)header[1] << 8));
    frameSize = (uint16_t)(header[2] | ((uint16_t)header[3] << 8));
    rxStatus = (uint16_t)(header[4] | ((uint16_t)header[5] << 8));

    /* Для ревизии B7 Next Packet Pointer должен быть чётным. */
    if (nextPacket > ENC28J60_RX_BUF_END ||
        (nextPacket & 1) != 0)
    {
        /* После сброса RX старые пакеты отбрасываются. */
        if (ENC28J60_RecoverRx() != 1)
            return ENC28J60_RX_SPI_ERROR;
        return ENC28J60_RX_BAD_POINTER;
    }

    if ((rxStatus & ENC28J60_RX_STATUS_OK) != 0 &&
        frameSize >= (ENC28J60_FRAME_MIN + ENC28J60_CRC_SIZE) &&
        frameSize <= ENC28J60_RX_FRAME_MAX)
    {
        dataSize = frameSize - ENC28J60_CRC_SIZE;
        if (dataSize > capacity)
            result = ENC28J60_RX_BUFFER_SMALL;
        else
        {
            if (ENC28J60_ReadBufferMem(data, dataSize) != HAL_OK)
                return ENC28J60_RX_SPI_ERROR;
            *receivedLen = dataSize;
            result = ENC28J60_RX_OK;
        }
    }

    /* Errata B7: записываем в ERXRDPT только нечётный адрес. */
    freePtr = (nextPacket == ENC28J60_RX_BUF_START)
                ? ENC28J60_RX_BUF_END : (uint16_t)(nextPacket - 1);
    if (ENC28J60_WriteControlRegPair(ENC28J60_REG_ERXRDPTL,
                                     freePtr) != HAL_OK)
        return ENC28J60_RX_SPI_ERROR;
    enc_next_packet = nextPacket;
    if (ENC28J60_BitFieldSetRaw(ENC28J60_REG_ECON2,
                               ENC28J60_ECON2_PKTDEC) != HAL_OK)
        return ENC28J60_RX_SPI_ERROR;
    return result;
}
uint16_t ENC28J60_GetPHYStatus(void)
{
    uint16_t status = 0;
    if (ENC28J60_ReadPHY(
        ENC28J60_PHY_PHSTAT2,
        &status) != 1)
    {
        return 0;
    }
    return status;
}
