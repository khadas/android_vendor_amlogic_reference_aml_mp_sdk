/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_DvbSource"
#include <utils/AmlMpLog.h>
#include <Aml_MP/Common.h>
#include "DvbSource.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

///////////////////////////////////////////////////////////////////////////////
typedef int (*DvbLockHandler)(int fendFd, const dmd_delivery_t* pDelivery);
static DvbLockHandler getProtoHandler(const char* proto);
static dmd_device_type_t getDeviceType(const char* proto);
static int setFendProp(int fendFd, const struct dtv_properties* prop);
static int lockDvb_T(int fendFd, const dmd_delivery_t* pDelivery);
static int lockDvb_C(int fendFd, const dmd_delivery_t* pDelivery);

static void getDefaultDeliveryConf(dmd_device_type_t type, dmd_delivery_t* delivery)
{
    memset(delivery, 0, sizeof(*delivery));

    switch (type) {
    case DMD_TERRESTRIAL:
        delivery->delivery.terrestrial.dvb_type = DMD_DVBTYPE_DVBT;
        delivery->delivery.terrestrial.desc.dvbt.frequency = 474*1000;
        delivery->delivery.terrestrial.desc.dvbt.bandwidth = DMD_BANDWIDTH_8M;
        delivery->delivery.terrestrial.desc.dvbt.transmission_mode = DMD_TRANSMISSION_8K;
        delivery->delivery.terrestrial.desc.dvbt.guard_interval = DMD_GUARD_INTERVAL_1_32;
        break;

    case DMD_CABLE:
        delivery->device_type = DMD_CABLE;
        delivery->delivery.cable.frequency = 474*1000;
        delivery->delivery.cable.symbol_rate = 6875;
        delivery->delivery.cable.modulation = DMD_MOD_64QAM;
        break;

    case DMD_SATELLITE:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
DvbSource::DvbSource(const char* proto, const char* address, const InputParameter& inputParameter, uint32_t flags)
: Source(inputParameter, flags)
, mProto(proto)
, mAddress(address)
, mDeviceType((dmd_device_type_t)0)
, mDelivery{}
{
    MLOGI("proto:%s, address:%s, demuxId:%d, programNumber:%d, sourceId:%d",
            proto, address, inputParameter.demuxId, inputParameter.programNumber, inputParameter.sourceId);
}

DvbSource::~DvbSource()
{
    stop();
}

int DvbSource::initCheck()
{
    if (getProtoHandler(mProto.c_str()) == nullptr) {
        MLOGE("unknown proto:%s", mProto.c_str());
        return -1;
    }

    mDeviceType = getDeviceType(mProto.c_str());

    char* address = strdup(mAddress.c_str());
    if (address == nullptr) {
        MLOGE("dup address failed!");
        return -1;
    }

    getDefaultDeliveryConf(mDeviceType, &mDelivery);


    const char* delim = "/";
    int index = 0;
    for (char* token = strsep(&address, delim); token != nullptr; token = strsep(&address, delim)) {
        if (strcmp(token, "") == 0) {
            continue;
        }

        switch (mDeviceType) {
        case DMD_TERRESTRIAL:
        {
            switch (index) {
            //freq
            case 0:
            {
                double freq = std::stod(token);
                MLOGI("freq: %.2f", freq);
                mDelivery.delivery.terrestrial.desc.dvbt.frequency = freq * 1000;
                break;
            }

            //bandwidth
            case 1:
            {
                MLOGI("bandwith:%s", token);
                if (strcasecmp(token, "8M") == 0) {
                    mDelivery.delivery.terrestrial.desc.dvbt.bandwidth = DMD_BANDWIDTH_8M;
                } else if (strcasecmp(token, "7M") == 0) {
                    mDelivery.delivery.terrestrial.desc.dvbt.bandwidth = DMD_BANDWIDTH_7M;
                } else if (strcasecmp(token, "6M") == 0) {
                    mDelivery.delivery.terrestrial.desc.dvbt.bandwidth = DMD_BANDWIDTH_6M;
                } else if (strcasecmp(token, "5M") == 0) {
                    mDelivery.delivery.terrestrial.desc.dvbt.bandwidth = DMD_BANDWIDTH_5M;
                }
                break;
            }

            default:
                break;
            }
        }
        break;

        case DMD_CABLE:
        {
            switch (index) {
            //freq
            case 0:
            {
                double freq = std::stod(token);
                MLOGI("freq: %.2f", freq);
                mDelivery.delivery.cable.frequency = freq *1000;
                break;
            }

            //symbol rate
            case 1:
            {
                double symbolRate = std::stod(token);
                MLOGI("symbol rate: %.2f", symbolRate);
                mDelivery.delivery.cable.symbol_rate = symbolRate;
                break;
            }

            //modulation
            case 2:
            {
                MLOGI("modulation: %s", token);
                if (strcasecmp(token, "16qam") == 0) {
                    mDelivery.delivery.cable.modulation = DMD_MOD_16QAM;
                } else if (strcasecmp(token, "32qam") == 0) {
                    mDelivery.delivery.cable.modulation = DMD_MOD_32QAM;
                } else if (strcasecmp(token, "64qam") == 0) {
                    mDelivery.delivery.cable.modulation = DMD_MOD_64QAM;
                } else if (strcasecmp(token, "128qam") == 0) {
                    mDelivery.delivery.cable.modulation = DMD_MOD_128QAM;
                } else if (strcasecmp(token, "256qam") == 0) {
                    mDelivery.delivery.cable.modulation = DMD_MOD_256QAM;
                }
                break;
            }
            }
        }
        break;

        default:
            break;
        }

        ++index;
    }

    free(address);

    int fendIndex = mInputParameter.fendId;
    MLOGI("open frontend dev, id:%d", fendIndex);
    if (openFend(fendIndex) < 0) {
        MLOGE("openFend failed!\n");
        return -1;
    }

    return 0;
}

int DvbSource::start()
{
    DvbLockHandler lockHandler = getProtoHandler(mProto.c_str());
    int ret = lockHandler(mFendFd, &mDelivery);
    if (ret < 0) {
        MLOGE("dvb lock (%s) failed!", mProto.c_str());
        return -1;
    }

    FendLockState fendState = FEND_STATE_UNKNOWN;
    int times = 0;
    while ((fendState = queryFendLockState()) == FEND_STATE_UNKNOWN || fendState == FEND_STATE_LOCK_TIMEOUT) {
        if (mRequestQuit) {
            break;
        }
        //Wait for the frequency lock to succeed 6s
        if (times < 60) {
            times++;
            usleep(100 * 1000);
        } else {
            break;
        }
    }

    MLOGI("start, fendState:%d", fendState);
    if (fendState != FEND_STATE_LOCKED) {
        return -1;
    }
    return 0;
}

int DvbSource::stop()
{
    closeFend();

    return 0;
}

void DvbSource::signalQuit()
{
    mRequestQuit = true;
}

///////////////////////////////////////////////////////////////////////////////
int DvbSource::openFend(int fendIndex)
{
   char fe_name[24];
   struct stat file_status;

   snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", fendIndex);

   if ((mFendFd = open(fe_name, O_RDWR | O_NONBLOCK)) < 0) {
      MLOGE("Failed to open [%s], errno %d\n", fe_name, errno);
      return -1;
   } else {
      MLOGE("Open %s frontend_fd:%d \n", fe_name, mFendFd);
   }

   return 0;
}

void DvbSource::closeFend()
{
    MLOGI("closeFend! mFendFd:%d", mFendFd);

    if (mFendFd < 0)
        return;

    struct dtv_properties props;
    int cmd_num = 0;
    struct dtv_property p[DTV_IOCTL_MAX_MSGS];
    p[cmd_num].cmd = DTV_DELIVERY_SYSTEM;
    p[cmd_num].u.data = SYS_UNDEFINED;
    cmd_num++;

    props.num = cmd_num;
    props.props = (struct dtv_property*)p;
    setFendProp(mFendFd, &props);

    close(mFendFd);
    mFendFd = -1;
}

DvbSource::FendLockState DvbSource::queryFendLockState() const
{
    fe_status_t feStatus;

    if (ioctl(mFendFd, FE_READ_STATUS, &feStatus) < 0) {
        printf("mfendf:%d FE_READ_STATUS errno:%d \n" ,mFendFd, errno);
        return FEND_STATE_ERROR;
    }

    MLOGI("current tuner status=0x%02x \n", feStatus);
   if ((feStatus& FE_HAS_LOCK) != 0) {
       printf("current tuner status [locked]\n");
       return FEND_STATE_LOCKED;
   } else if ((feStatus& FE_TIMEDOUT) != 0) {
        printf("current tuner status [unlocked]\n");
        return FEND_STATE_LOCK_TIMEOUT;
   }

   return FEND_STATE_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////
struct ProtoItem {
    const char* proto;
    dmd_device_type_t type;
    int (*fn)(int, const dmd_delivery_t*);
};

static ProtoItem g_protoList[] = {
    {"dvbt", DMD_TERRESTRIAL, &lockDvb_T},
    {"dvbc", DMD_CABLE, &lockDvb_C},
    {nullptr, (dmd_device_type_t)0, nullptr},
};

static DvbLockHandler getProtoHandler(const char* proto)
{
    ProtoItem *item = g_protoList;

    for (; item->proto; item++) {
        if (!strcmp(item->proto, proto)) {
            return item->fn;
        }
    }

    return nullptr;
}

static dmd_device_type_t getDeviceType(const char* proto)
{
    ProtoItem *item = g_protoList;

    for (; item->proto; item++) {
        if (!strcmp(item->proto, proto)) {
            return item->type;
        }
    }

    return (dmd_device_type_t)0;
}

static int setFendProp(int fendFd, const struct dtv_properties *prop)
{
  if (ioctl(fendFd, FE_SET_PROPERTY, prop) == -1) {
     printf("set prop failed>>>>>>>>>>>>.\n");
     return -1;
  }

  return 0;
}

static int lockDvb_T(int fendFd, const dmd_delivery_t * pDelivery)
{
    MLOGI("%s", __FUNCTION__);

   int tmp = 0;
   int cmd_num = 0;
   struct dtv_properties props;
   struct dtv_property p[DTV_IOCTL_MAX_MSGS];

   p[cmd_num].cmd = DTV_DELIVERY_SYSTEM;
   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 p[cmd_num].u.data = SYS_DVBT;
   else
	 p[cmd_num].u.data = SYS_DVBT2;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 p[cmd_num].u.data = pDelivery->delivery.terrestrial.desc.dvbt.frequency * 1000;
   else
	 p[cmd_num].u.data = pDelivery->delivery.terrestrial.desc.dvbt2.frequency * 1000;

   p[cmd_num].cmd = DTV_FREQUENCY;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt.bandwidth;
   else
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt2.bandwidth;
   p[cmd_num].cmd = DTV_BANDWIDTH_HZ;
   switch (tmp) {
   case DMD_BANDWIDTH_10M:
	 p[cmd_num].u.data = 10000000;
	 break;
   case DMD_BANDWIDTH_8M:
	 p[cmd_num].u.data = 8000000;
	 break;
   case DMD_BANDWIDTH_7M:
	 p[cmd_num].u.data = 7000000;
	 break;
   case DMD_BANDWIDTH_6M:
	 p[cmd_num].u.data = 6000000;
	 break;
   case DMD_BANDWIDTH_5M:
	 p[cmd_num].u.data = 5000000;
	 break;
   case DMD_BANDWIDTH_17M:
	 p[cmd_num].u.data = 1712000;
	 break;
   }
   cmd_num++;

   p[cmd_num].cmd = DTV_CODE_RATE_HP;
   p[cmd_num].u.data = FEC_AUTO;
   cmd_num++;

   p[cmd_num].cmd = DTV_CODE_RATE_LP;
   p[cmd_num].u.data = FEC_AUTO;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt.transmission_mode;
   else
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt2.transmission_mode;
   if (tmp <= DMD_TRANSMISSION_8K)
	 tmp += -1;
   p[cmd_num].cmd = DTV_TRANSMISSION_MODE;
   p[cmd_num].u.data = tmp;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT)
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt.guard_interval;
   else
	 tmp = pDelivery->delivery.terrestrial.desc.dvbt2.guard_interval;
   if (tmp <= DMD_GUARD_INTERVAL_1_4)
	 tmp += -1;
   p[cmd_num].cmd = DTV_GUARD_INTERVAL;
   p[cmd_num].u.data = tmp;
   cmd_num++;

   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT) {
	 p[cmd_num].cmd = DTV_HIERARCHY;
	 p[cmd_num].u.data = HIERARCHY_AUTO;
	 cmd_num++;
   }
   if (pDelivery->delivery.terrestrial.dvb_type == DMD_DVBTYPE_DVBT2) {
	 p[cmd_num].cmd = DTV_DVBT2_PLP_ID_LEGACY;
	 p[cmd_num].u.data = pDelivery->delivery.terrestrial.desc.dvbt2.plp_id;
	 cmd_num++;
   }

   p[cmd_num].cmd = DTV_TUNE;
   cmd_num++;
   props.num = cmd_num;
   props.props = (struct dtv_property *)&p;

   return setFendProp(fendFd, &props);
}

static int lockDvb_C(int fendFd, const dmd_delivery_t * pDelivery)
{
    MLOGI("%s", __FUNCTION__);

   int tmp = 0;
   int cmd_num = 0;
   struct dtv_properties props;
   struct dtv_property p[DTV_IOCTL_MAX_MSGS];

   p[cmd_num].cmd = DTV_DELIVERY_SYSTEM;
   p[cmd_num].u.data = SYS_DVBC_ANNEX_A;
   cmd_num++;
   p[cmd_num].cmd = DTV_FREQUENCY;
   p[cmd_num].u.data = pDelivery->delivery.cable.frequency * 1000;
   cmd_num++;

   p[cmd_num].cmd = DTV_SYMBOL_RATE;
   p[cmd_num].u.data = pDelivery->delivery.cable.symbol_rate * 1000;
   cmd_num++;

   tmp = pDelivery->delivery.cable.modulation;
   switch (tmp) {
   case DMD_MOD_NONE:
	 tmp = QAM_AUTO;
	 break;
   case DMD_MOD_QPSK:
	 tmp = QPSK;
	 break;
   case DMD_MOD_8PSK:
	 tmp = PSK_8;
	 break;
   case DMD_MOD_QAM:
	 tmp = QAM_AUTO;
	 break;
   case DMD_MOD_4QAM:
	 tmp = QAM_AUTO;
	 break;
   case DMD_MOD_16QAM:
	 tmp = QAM_16;
	 break;
   case DMD_MOD_32QAM:
	 tmp = QAM_32;
	 break;
   case DMD_MOD_64QAM:
	 tmp = QAM_64;
	 break;
   case DMD_MOD_128QAM:
	 tmp = QAM_128;
	 break;
   case DMD_MOD_256QAM:
	 tmp = QAM_256;
	 break;
   case DMD_MOD_BPSK:
   case DMD_MOD_ALL:
	 tmp = QAM_AUTO;
	 break;
   }
   p[cmd_num].cmd = DTV_MODULATION;
   p[cmd_num].u.data = tmp;
   cmd_num++;

   p[cmd_num].cmd = DTV_TUNE;
   cmd_num++;
   props.num = cmd_num;
   props.props = (struct dtv_property *)&p;

   return setFendProp(fendFd, &props);
}

}
