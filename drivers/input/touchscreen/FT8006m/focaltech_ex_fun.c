/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2017, Focaltech Ltd. All rights reserved.
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
*
* File Name: Focaltech_ex_fun.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "focaltech_core.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
/*create apk debug channel*/
#define PROC_UPGRADE                            0
#define PROC_READ_REGISTER                      1
#define PROC_WRITE_REGISTER                     2
#define PROC_AUTOCLB                            4
#define PROC_UPGRADE_INFO                       5
#define PROC_WRITE_DATA                         6
#define PROC_READ_DATA                          7
#define PROC_SET_TEST_FLAG                      8
#define PROC_SET_SLAVE_ADDR                     10
#define PROC_HW_RESET                           11
#define PROC_NAME                               "ftxxxx-debug"
#define WRITE_BUF_SIZE                          512
#define READ_BUF_SIZE                           512

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/

/*****************************************************************************
* Static variables
*****************************************************************************/
static unsigned char proc_operate_mode = PROC_UPGRADE;
static struct proc_dir_entry *fts_proc_entry;
static struct
{
    int op;
    int reg;
    int value;
    int result;
} g_rwreg_result;

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
/*interface of write proc*/
/************************************************************************
*   Name: fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static ssize_t fts_debug_write(struct file *filp, const char __user *buff, size_t count, loff_t *ppos)
{
    unsigned char writebuf[WRITE_BUF_SIZE];
    int buflen = count;
    int writelen = 0;
    int ret = 0;
    char tmp[25];

    if (copy_from_user(&writebuf, buff, buflen))
    {
        return -EFAULT;
    }

    proc_operate_mode = writebuf[0];
    switch (proc_operate_mode)
    {
        case PROC_UPGRADE:
        {
            char upgrade_file_path[FILE_NAME_LENGTH];
            memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
            sprintf(upgrade_file_path, "%s", writebuf + 1);
            upgrade_file_path[buflen-1] = '\0';
            ft8006m_irq_disable();
            if (ft8006m_updatefun_curr.upgrade_with_app_bin_file)
                ret = ft8006m_updatefun_curr.upgrade_with_app_bin_file(ft8006m_i2c_client, upgrade_file_path);
            ft8006m_irq_enable();
            if (ret < 0)
            {
                FTS_ERROR("[APK]: upgrade failed!!");
            }
        }
        break;

        case PROC_SET_TEST_FLAG:
            break;
        case PROC_READ_REGISTER:
            writelen = 1;
            ret = ft8006m_i2c_write(ft8006m_i2c_client, writebuf + 1, writelen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: write iic error!!");
            }
            break;
        case PROC_WRITE_REGISTER:
            writelen = 2;
            ret = ft8006m_i2c_write(ft8006m_i2c_client, writebuf + 1, writelen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: write iic error!!");
            }
            break;
        case PROC_SET_SLAVE_ADDR:

            ret = ft8006m_i2c_client->addr;
            if (writebuf[1] != ft8006m_i2c_client->addr)
            {
                ft8006m_i2c_client->addr = writebuf[1];

            }
            break;

        case PROC_HW_RESET:

            sprintf(tmp, "%s", writebuf + 1);
            tmp[buflen - 1] = '\0';
            if (strncmp(tmp, "focal_driver", 12) == 0)
            {
                ft8006m_reset_proc(1);
            }

            break;

        case PROC_AUTOCLB:
            ft8006m_ctpm_auto_clb(ft8006m_i2c_client);
            break;
        case PROC_READ_DATA:
        case PROC_WRITE_DATA:
            writelen = count - 1;
            if (writelen > 0)
            {
                ret = ft8006m_i2c_write(ft8006m_i2c_client, writebuf + 1, writelen);
                if (ret < 0)
                {
                    FTS_ERROR("[APK]: write iic error!!");
                }
            }
            break;
        default:
            break;
    }

    if (ret < 0)
    {
        return ret;
    }
    else
    {
        return count;
    }
}

/* interface of read proc */
/************************************************************************
*   Name: fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use
* Output: page point to data
* Return: read char number
***********************************************************************/
static ssize_t fts_debug_read(struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
    int ret = 0;
    int num_read_chars = 0;
    int readlen = 0;
    u8 regvalue = 0x00, regaddr = 0x00;
    unsigned char buf[READ_BUF_SIZE];

    switch (proc_operate_mode)
    {
        case PROC_UPGRADE:

            regaddr = FTS_REG_FW_VER;
            ret = ft8006m_i2c_read_reg(ft8006m_i2c_client, regaddr, &regvalue);
            if (ret < 0)
                num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
            else
                num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
            break;
        case PROC_READ_REGISTER:
            readlen = 1;
            ret = ft8006m_i2c_read(ft8006m_i2c_client, NULL, 0, buf, readlen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: read iic error!!");
                return ret;
            }
            num_read_chars = 1;
            break;
        case PROC_READ_DATA:
            readlen = count;
            ret = ft8006m_i2c_read(ft8006m_i2c_client, NULL, 0, buf, readlen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: read iic error!!");
                return ret;
            }

            num_read_chars = readlen;
            break;
        case PROC_WRITE_DATA:
            break;
        default:
            break;
    }

    if (copy_to_user(buff, buf, num_read_chars))
    {
        FTS_ERROR("[APK]: copy to user error!!");
        return -EFAULT;
    }

    return num_read_chars;
}
static const struct file_operations fts_proc_fops =
{
    .owner  = THIS_MODULE,
    .read   = fts_debug_read,
    .write  = fts_debug_write,
};
#else
/* interface of write proc */
/************************************************************************
*   Name: fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static int fts_debug_write(struct file *filp,
                           const char __user *buff, unsigned long len, void *data)
{
    unsigned char writebuf[WRITE_BUF_SIZE];
    int buflen = len;
    int writelen = 0;
    int ret = 0;
    char tmp[25];

    if (copy_from_user(&writebuf, buff, buflen))
    {
        FTS_ERROR("[APK]: copy from user error!!");
        return -EFAULT;
    }

    proc_operate_mode = writebuf[0];
    switch (proc_operate_mode)
    {

        case PROC_UPGRADE:
        {
            char upgrade_file_path[FILE_NAME_LENGTH];
            memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
            sprintf(upgrade_file_path, "%s", writebuf + 1);
            upgrade_file_path[buflen-1] = '\0';
            ft8006m_irq_disable();
            if (ft8006m_updatefun_curr.upgrade_with_app_bin_file)
                ret = ft8006m_updatefun_curr.upgrade_with_app_bin_file(ft8006m_i2c_client, upgrade_file_path);
            ft8006m_irq_enable();
            if (ret < 0)
            {
                FTS_ERROR("[APK]: upgrade failed!!");
            }
        }
        break;
        case PROC_SET_TEST_FLAG:
            break;
        case PROC_READ_REGISTER:
            writelen = 1;
            ret = ft8006m_i2c_write(ft8006m_i2c_client, writebuf + 1, writelen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: write iic error!!n");
            }
            break;
        case PROC_WRITE_REGISTER:
            writelen = 2;
            ret = ft8006m_i2c_write(ft8006m_i2c_client, writebuf + 1, writelen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: write iic error!!");
            }
            break;
        case PROC_SET_SLAVE_ADDR:

            ret = ft8006m_i2c_client->addr;
            if (writebuf[1] != ft8006m_i2c_client->addr)
            {
                ft8006m_i2c_client->addr = writebuf[1];
            }
            break;

        case PROC_HW_RESET:

            sprintf(tmp, "%s", writebuf + 1);
            tmp[buflen - 1] = '\0';
            if (strncmp(tmp, "focal_driver", 12) == 0)
            {
                ft8006m_reset_proc(1);
            }

            break;

        case PROC_AUTOCLB:
            ft8006m_ctpm_auto_clb(ft8006m_i2c_client);
            break;
        case PROC_READ_DATA:
        case PROC_WRITE_DATA:
            writelen = len - 1;
            if (writelen > 0)
            {
                ret = ft8006m_i2c_write(ft8006m_i2c_client, writebuf + 1, writelen);
                if (ret < 0)
                {
                    FTS_ERROR("[APK]: write iic error!!");
                }
            }
            break;
        default:
            break;
    }

    if (ret < 0)
    {
        return ret;
    }
    else
    {
        return len;
    }
}

/* interface of read proc */
/************************************************************************
*   Name: fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use
* Output: page point to data
* Return: read char number
***********************************************************************/
static int fts_debug_read(char *page, char **start,
                           off_t off, int count, int *eof, void *data)
{
    int ret = 0;
    unsigned char buf[READ_BUF_SIZE];
    int num_read_chars = 0;
    int readlen = 0;
    u8 regvalue = 0x00, regaddr = 0x00;

    switch (proc_operate_mode)
    {
        case PROC_UPGRADE:

            regaddr = FTS_REG_FW_VER;
            ret = ft8006m_i2c_read_reg(ft8006m_i2c_client, regaddr, &regvalue);
            if (ret < 0)
                num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
            else
                num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
            break;
        case PROC_READ_REGISTER:
            readlen = 1;
            ret = ft8006m_i2c_read(ft8006m_i2c_client, NULL, 0, buf, readlen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: read iic error!!");
                return ret;
            }
            num_read_chars = 1;
            break;
        case PROC_READ_DATA:
            readlen = count;
            ret = ft8006m_i2c_read(ft8006m_i2c_client, NULL, 0, buf, readlen);
            if (ret < 0)
            {
                FTS_ERROR("[APK]: read iic error!!");
                return ret;
            }

            num_read_chars = readlen;
            break;
        case PROC_WRITE_DATA:
            break;
        default:
            break;
    }

    memcpy(page, buf, num_read_chars);
    return num_read_chars;
}
#endif
/************************************************************************
* Name: ft8006m_create_apk_debug_channel
* Brief:  create apk debug channel
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/
int ft8006m_create_apk_debug_channel(struct i2c_client *client)
{
    fts_proc_entry = proc_create(PROC_NAME, 0777, NULL, &fts_proc_fops);
    if (NULL == fts_proc_entry)
    {
        FTS_ERROR("Couldn't create proc entry!");
        return -ENOMEM;
    }
    return 0;
}
/************************************************************************
* Name: ft8006m_release_apk_debug_channel
* Brief:  release apk debug channel
* Input: no
* Output: no
* Return: no
***********************************************************************/
void ft8006m_release_apk_debug_channel(void)
{

    if (fts_proc_entry)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
        proc_remove(fts_proc_entry);
#else
        remove_proc_entry(PROC_NAME, NULL);
#endif
}

/*
 * fts_hw_reset interface
 */
static ssize_t fts_hw_reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}
static ssize_t fts_hw_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t count = 0;

    ft8006m_reset_proc(200);

    count = snprintf(buf, PAGE_SIZE, "hw reset executed\n");

    return count;
}

/*
 * fts_irq interface
 */
static ssize_t fts_irq_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    if (FTS_SYSFS_ECHO_ON(buf))
    {
        ft8006m_irq_enable();
    }
    else if (FTS_SYSFS_ECHO_OFF(buf))
    {
        ft8006m_irq_disable();
    }
    return count;
}

static ssize_t fts_irq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return -EPERM;
}

/************************************************************************
* Name: fts_tpfwver_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t fts_tpfwver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t num_read_chars = 0;
    u8 fwver = 0;

    mutex_lock(&ft8006m_input_dev->mutex);

    if (ft8006m_i2c_read_reg(ft8006m_i2c_client, FTS_REG_FW_VER, &fwver) < 0)
    {
        num_read_chars = snprintf(buf, PAGE_SIZE, "I2c transfer error!\n");
    }
    if (fwver == 255)
        num_read_chars = snprintf(buf, PAGE_SIZE, "get tp fw version fail!\n");
    else
    {
        num_read_chars = snprintf(buf, PAGE_SIZE, "%02X\n", fwver);
    }

    mutex_unlock(&ft8006m_input_dev->mutex);

    return num_read_chars;
}
/************************************************************************
* Name: fts_tpfwver_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tpfwver_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    /* place holder for future use */
    return -EPERM;
}

static int fts_is_hex_char(const char ch)
{
    int result = 0;
    if (ch >= '0' && ch <= '9')
    {
        result = 1;
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        result = 1;
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int fts_hex_char_to_int(const char ch)
{
    int result = 0;
    if (ch >= '0' && ch <= '9')
    {
        result = (int)(ch - '0');
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        result = (int)(ch - 'a') + 10;
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        result = (int)(ch - 'A') + 10;
    }
    else
    {
        result = -1;
    }

    return result;
}

static int fts_hex_to_str(char *hex, int iHexLen, char *ch, int *iChLen)
{
    int high = 0;
    int low = 0;
    int tmp = 0;
    int i = 0;
    int iCharLen = 0;
    if (hex == NULL || ch == NULL)
    {
        return -EPERM;
    }

    if (iHexLen %2 == 1)
    {
        return -ENOENT;
    }

    for (i = 0; i < iHexLen; i += 2)
    {
        high = fts_hex_char_to_int(hex[i]);
        if (high < 0)
        {
            ch[iCharLen] = '\0';
            return -3;
        }

        low = fts_hex_char_to_int(hex[i+1]);
        if (low < 0)
        {
            ch[iCharLen] = '\0';
            return -3;
        }
        tmp = (high << 4) + low;
        ch[iCharLen++] = (char)tmp;
    }
    ch[iCharLen] = '\0';
    *iChLen = iCharLen;
    return 0;
}

static void fts_str_to_bytes(char  *bufStr, int iLen, char *uBytes, int *iBytesLen)
{
    int i = 0;
    int iNumChLen = 0;

    *iBytesLen = 0;

    for (i = 0; i < iLen; i++)
    {
        if (fts_is_hex_char(bufStr[i]))
        {
            bufStr[iNumChLen++] = bufStr[i];
        }
    }

    bufStr[iNumChLen] = '\0';

    fts_hex_to_str(bufStr, iNumChLen, uBytes, iBytesLen);
}
/************************************************************************
* Name: fts_tprwreg_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tprwreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count;

    mutex_lock(&ft8006m_input_dev->mutex);

    if (!g_rwreg_result.op)
    {
        if (g_rwreg_result.result == 0)
        {
            count = sprintf(buf, "Read %02X: %02X\n", g_rwreg_result.reg, g_rwreg_result.value);
        }
        else
        {
            count = sprintf(buf, "Read %02X failed, ret: %d\n", g_rwreg_result.reg,  g_rwreg_result.result);
        }
    }
    else
    {
        if (g_rwreg_result.result == 0)
        {
            count = sprintf(buf, "Write %02X, %02X success\n", g_rwreg_result.reg,  g_rwreg_result.value);
        }
        else
        {
            count = sprintf(buf, "Write %02X failed, ret: %d\n", g_rwreg_result.reg,  g_rwreg_result.result);
        }
    }
    mutex_unlock(&ft8006m_input_dev->mutex);

    return count;
}
/************************************************************************
* Name: fts_tprwreg_store
* Brief:  read/write register
* Input: device, device attribute, char buf, char count
* Output: print register value
* Return: char count
***********************************************************************/
static ssize_t fts_tprwreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);
    ssize_t num_read_chars = 0;
    int retval;
    long unsigned int wmreg = 0;
    u8 regaddr = 0xff, regvalue = 0xff;
    u8 valbuf[5] = {0};

    memset(valbuf, 0, sizeof(valbuf));
    mutex_lock(&ft8006m_input_dev->mutex);
    num_read_chars = count - 1;
    if (num_read_chars != 2)
    {
        if (num_read_chars != 4)
        {
            FTS_ERROR("please input 2 or 4 character");
            goto error_return;
        }
    }
    memcpy(valbuf, buf, num_read_chars);
    retval = kstrtoul(valbuf, 16, &wmreg);
    fts_str_to_bytes((char*)buf, num_read_chars, valbuf, &retval);

    if (1 == retval)
    {
        regaddr = valbuf[0];
        retval = 0;
    }
    else if (2 == retval)
    {
        regaddr = valbuf[0];
        regvalue = valbuf[1];
        retval = 0;
    }
    else
        retval = -1;

    if (0 != retval)
    {
        FTS_ERROR("%s() - ERROR: Could not convert the given input to a number. The given input was: \"%s\"", __FUNCTION__, buf);
        goto error_return;
    }

    if (2 == num_read_chars)
    {
        g_rwreg_result.op = 0;
        g_rwreg_result.reg = regaddr;
        /*read register*/
        regaddr = wmreg;
        g_rwreg_result.result = ft8006m_i2c_read_reg(client, regaddr, &regvalue);
        if (g_rwreg_result.result < 0)
        {
            FTS_ERROR("Could not read the register(0x%02x)", regaddr);
        }
        else
        {
            g_rwreg_result.value = regvalue;
            g_rwreg_result.result = 0;
        }
    }
    else
    {
        regaddr = wmreg>>8;
        regvalue = wmreg;

        g_rwreg_result.op = 1;
        g_rwreg_result.reg = regaddr;
        g_rwreg_result.value = regvalue;
        g_rwreg_result.result = ft8006m_i2c_write_reg(client, regaddr, regvalue);
        if (g_rwreg_result.result < 0)
        {
            FTS_ERROR("Could not write the register(0x%02x)", regaddr);

        }
        else
        {
            g_rwreg_result.result = 0;
        }
    }

error_return:
    mutex_unlock(&ft8006m_input_dev->mutex);

    return count;
}
/************************************************************************
* Name: fts_fwupdate_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_fwupdate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    /* place holder for future use */
    return -EPERM;
}

/************************************************************************
* Name: fts_fwupdate_store
* Brief:  upgrade from *.i
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t fts_fwupdate_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    mutex_lock(&ft8006m_input_dev->mutex);
    ft8006m_irq_disable();

    if (ft8006m_updatefun_curr.upgrade_with_app_i_file)
        ft8006m_updatefun_curr.upgrade_with_app_i_file(client);
#if FTS_AUTO_UPGRADE_FOR_LCD_CFG_EN
    if (ft8006m_updatefun_curr.upgrade_with_lcd_cfg_i_file)
        ft8006m_updatefun_curr.upgrade_with_lcd_cfg_i_file(client);
#endif

    ft8006m_irq_enable();
    mutex_unlock(&ft8006m_input_dev->mutex);

    return count;
}
/************************************************************************
* Name: fts_fwupgradeapp_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_fwupgradeapp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    /* place holder for future use */
    return -EPERM;
}

/************************************************************************
* Name: fts_fwupgradeapp_store
* Brief:  upgrade from app.bin
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t fts_fwupgradeapp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char fwname[FILE_NAME_LENGTH];
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    memset(fwname, 0, sizeof(fwname));
    sprintf(fwname, "%s", buf);
    fwname[count-1] = '\0';

    mutex_lock(&ft8006m_input_dev->mutex);
    ft8006m_irq_disable();

    if (ft8006m_updatefun_curr.upgrade_with_app_bin_file)
        ft8006m_updatefun_curr.upgrade_with_app_bin_file(client, fwname);

    ft8006m_irq_enable();
    mutex_unlock(&ft8006m_input_dev->mutex);

    return count;
}
/************************************************************************
* Name: fts_driverversion_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_driverversion_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count;

    mutex_lock(&ft8006m_input_dev->mutex);

    count = sprintf(buf, FTS_DRIVER_VERSION "\n");

    mutex_unlock(&ft8006m_input_dev->mutex);

    return count;
}
/************************************************************************
* Name: fts_driverversion_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_driverversion_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    /* place holder for future use */
    return -EPERM;
}

/************************************************************************
* Name: fts_module_config_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_module_config_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;

    mutex_lock(&ft8006m_input_dev->mutex);

    count += sprintf(buf, "FTS_CHIP_TYPE: \t\t\t%04X\n", FTS_CHIP_TYPE);
#if defined(FTS_MT_PROTOCOL_B_EN)
    count += sprintf(buf+count, "TS_MT_PROTOCOL_B_EN: \t\t%s\n", FTS_MT_PROTOCOL_B_EN ? "ON" : "OFF");
#endif
    count += sprintf(buf+count, "FTS_GESTURE_EN: \t\t%s\n", FTS_GESTURE_EN ? "ON" : "OFF");
#if defined(FTS_PSENSOR_EN)
    count += sprintf(buf+count, "FTS_PSENSOR_EN: \t\t%s\n", FTS_PSENSOR_EN ? "ON" : "OFF");
#endif
    count += sprintf(buf+count, "FTS_GLOVE_EN: \t\t\t%s\n", FTS_GLOVE_EN ? "ON" : "OFF");
    count += sprintf(buf+count, "FTS_COVER_EN: \t\t%s\n", FTS_COVER_EN ? "ON" : "OFF");
    count += sprintf(buf+count, "FTS_CHARGER_EN: \t\t\t%s\n", FTS_CHARGER_EN ? "ON" : "OFF");

    count += sprintf(buf+count, "FTS_REPORT_PRESSURE_EN: \t\t%s\n", FTS_REPORT_PRESSURE_EN ? "ON" : "OFF");
    count += sprintf(buf+count, "FTS_FORCE_TOUCH_EN: \t\t%s\n", FTS_FORCE_TOUCH_EN ? "ON" : "OFF");

    count += sprintf(buf+count, "FTS_APK_NODE_EN: \t\t%s\n", FTS_APK_NODE_EN ? "ON" : "OFF");
    count += sprintf(buf+count, "FTS_POWER_SOURCE_CUST_EN: \t%s\n", FTS_POWER_SOURCE_CUST_EN ? "ON" : "OFF");
    count += sprintf(buf+count, "FTS_AUTO_UPGRADE_EN: \t\t%s\n", FTS_AUTO_UPGRADE_EN ? "ON" : "OFF");

    mutex_unlock(&ft8006m_input_dev->mutex);

    return count;
}
/************************************************************************
* Name: fts_module_config_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_module_config_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    /* place holder for future use */
    return -EPERM;
}

/************************************************************************
* Name: fts_dumpreg_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_dumpreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    /* place holder for future use */
    return -EPERM;
}

/************************************************************************
* Name: fts_dumpreg_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_dumpreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char tmp[256];
    int count = 0;
    u8 regvalue = 0;
    struct i2c_client *client;

    mutex_lock(&ft8006m_input_dev->mutex);

    client = container_of(dev, struct i2c_client, dev);
    ft8006m_i2c_read_reg(client, FTS_REG_POWER_MODE, &regvalue);
    count += sprintf(tmp + count, "Power Mode:0x%02x\n", regvalue);

    ft8006m_i2c_read_reg(client, FTS_REG_FW_VER, &regvalue);
    count += sprintf(tmp + count, "FW Ver:0x%02x\n", regvalue);

    ft8006m_i2c_read_reg(client, FTS_REG_VENDOR_ID, &regvalue);
    count += sprintf(tmp + count, "Vendor ID:0x%02x\n", regvalue);

    ft8006m_i2c_read_reg(client, FTS_REG_LCD_BUSY_NUM, &regvalue);
    count += sprintf(tmp + count, "LCD Busy Number:0x%02x\n", regvalue);

    ft8006m_i2c_read_reg(client, FTS_REG_GESTURE_EN, &regvalue);
    count += sprintf(tmp + count, "Gesture Mode:0x%02x\n", regvalue);

    ft8006m_i2c_read_reg(client, FTS_REG_CHARGER_MODE_EN, &regvalue);
    count += sprintf(tmp + count, "charge stat:0x%02x\n", regvalue);

    ft8006m_i2c_read_reg(client, FTS_REG_INT_CNT, &regvalue);
    count += sprintf(tmp + count, "INT count:0x%02x\n", regvalue);

    ft8006m_i2c_read_reg(client, FTS_REG_FLOW_WORK_CNT, &regvalue);
    count += sprintf(tmp + count, "ESD count:0x%02x\n", regvalue);

    memcpy(buf, tmp, count);
    mutex_unlock(&ft8006m_input_dev->mutex);
    return count;
}

/****************************************/
/* sysfs */
/* get the fw version
*   example:cat fw_version
*/
static DEVICE_ATTR(fts_fw_version, S_IRUGO|S_IWUSR, fts_tpfwver_show, fts_tpfwver_store);

/* upgrade from *.i
*   example: echo 1 > fw_update
*/
static DEVICE_ATTR(fts_fw_update, S_IRUGO|S_IWUSR, fts_fwupdate_show, fts_fwupdate_store);
/* read and write register
*   read example: echo 88 > rw_reg ---read register 0x88
*   write example:echo 8807 > rw_reg ---write 0x07 into register 0x88
*
*   note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(fts_rw_reg, S_IRUGO|S_IWUSR, fts_tprwreg_show, fts_tprwreg_store);
/*  upgrade from app.bin
*    example:echo "*_app.bin" > upgrade_app
*/
static DEVICE_ATTR(fts_upgrade_app, S_IRUGO|S_IWUSR, fts_fwupgradeapp_show, fts_fwupgradeapp_store);
static DEVICE_ATTR(fts_driver_version, S_IRUGO|S_IWUSR, fts_driverversion_show, fts_driverversion_store);
static DEVICE_ATTR(fts_dump_reg, S_IRUGO|S_IWUSR, fts_dumpreg_show, fts_dumpreg_store);
static DEVICE_ATTR(fts_module_config, S_IRUGO|S_IWUSR, fts_module_config_show, fts_module_config_store);
static DEVICE_ATTR(fts_hw_reset, S_IRUGO|S_IWUSR, fts_hw_reset_show, fts_hw_reset_store);
static DEVICE_ATTR(fts_irq, S_IRUGO|S_IWUSR, fts_irq_show, fts_irq_store);

/* add your attr in here*/
static struct attribute *fts_attributes[] =
{
    &dev_attr_fts_fw_version.attr,
    &dev_attr_fts_fw_update.attr,
    &dev_attr_fts_rw_reg.attr,
    &dev_attr_fts_dump_reg.attr,
    &dev_attr_fts_upgrade_app.attr,
    &dev_attr_fts_driver_version.attr,
    &dev_attr_fts_module_config.attr,
    &dev_attr_fts_hw_reset.attr,
    &dev_attr_fts_irq.attr,
    NULL
};

static struct attribute_group fts_attribute_group =
{
    .attrs = fts_attributes
};

/************************************************************************
* Name: ft8006m_create_sysfs
* Brief:  create sysfs for debug
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/
int ft8006m_create_sysfs(struct i2c_client *client)
{
    int err;
    err = sysfs_create_group(&client->dev.kobj, &fts_attribute_group);
    if (0 != err)
    {
        FTS_ERROR("[EX]: sysfs_create_group() failed!!");
        sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
        return -EIO;
    }
    return err;
}
/************************************************************************
* Name: ft8006m_remove_sysfs
* Brief:  remove sys
* Input: i2c info
* Output: no
* Return: no
***********************************************************************/
int ft8006m_remove_sysfs(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
    return 0;
}
