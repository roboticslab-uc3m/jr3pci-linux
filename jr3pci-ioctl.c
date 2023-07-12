#include "jr3pci-ioctl.h"

void __iomem *jr3_base_address;

/** Board address to PCI virtual address conversion */
void __iomem *board2virtual(int ba) {
	return (void*)jr3_base_address+4*ba;
}

/** Read data memory */
short readData(int ba, int card) {
	return (short)readl(board2virtual(ba+card*CARD_OFFSET));
}

/** Write data memory */
void writeData(int ba, int data, int card) {
	writel(data,board2virtual(ba+card*CARD_OFFSET));
}

void writeProgram(int pa, short data0, short data1, int card) {
	writew(data0,board2virtual(pa+card*CARD_OFFSET));
	writew(data1,(board2virtual(pa+card*CARD_OFFSET)+JR3_P8BIT_OFFSET));
}

/** Read program memory */
int readProgram(int pa, int card) {
	int r;
	r=readw(board2virtual(pa+card*CARD_OFFSET)) << 8;
	r=r|readb(board2virtual(pa+card*CARD_OFFSET)+JR3_P8BIT_OFFSET);
	return r;
}

int jr3Reset(int card) {
	writeData(JR3_RESET_ADDRESS,0,card);
	return 0;
}

int jr3ZeroOffs(int card) {
	writeData(JR3_COMMAND0,JR3_CMD_RESETOFFSETS,card);
	return 0;
}

int jr3Filter(unsigned long arg, int num_filter, int card) {
	int i;
	int ret=0;
	int axload[6];
	int address;

	for (i = 0; i < 6; i++) {
		address = JR3_FILTER0+0x08*num_filter+i;
		axload[i]= (short) readData(address, card);
	}

	ret = copy_to_user((void *) arg, (int *) axload, sizeof(six_axis_array));
	return ret;
}

/* Not tested */
int jr3SetFullScales(unsigned long arg, int card) {
	int fs[8];
	int ret=copy_from_user((int*) fs, (void *) arg, sizeof(force_array));
	int i;
	int address;

	for (i=0; i< 8; i++) {
		address=JR3_FULLSCALE+i;
		writeData(address,fs[i],card);
	}
	writeData(JR3_COMMAND0,JR3_CMD_SETFULLSCALES,card);
	return ret;
}

int jr3_open(struct inode *inode, struct file *filp) {
  //	MOD_INC_USE_COUNT;
	return 0;
}
int jr3_release(struct inode *inode, struct file *filp) {
  //	MOD_DEC_USE_COUNT;
	return 0;
}

int jr3GetFullScales(unsigned long arg, int card) {
  int i;
  int ret=0;
  int fullscales[8];

  for (i = 0; i < 8; i++)
	    fullscales[i]= readData(JR3_FULLSCALE+i, card);
  ret = copy_to_user((void *) arg, (int *) fullscales, sizeof(force_array));

  return ret;
}

int jr3GetForceAndRaw(unsigned long arg) {
	int i;
	int ret = 0;
	int data[70]; // (16 * 4) + 6

	for (i = 0; i < 16; i++) {
		data[(i * 4)] = readData(JR3_RAWCHANNELS + (i * 4), 0);
		data[(i * 4) + 1] = readData(JR3_RAWCHANNELS + (i * 4) + 1, 0);
		data[(i * 4) + 2] = 0;
		data[(i * 4) + 3] = 0;
	}

	for (i = 0; i < 6; i++) {
		data[i + 64] = readData(JR3_FILTER0 + i, 0);
	}

	ret = copy_to_user((void *) arg, (int *) data, sizeof(force_and_raw_array));
	return ret;
}

int jr3DumpDsp(void) {
	int i;
	int card;

	for (card = 0; card < 4; card++)
	{
		for (i = 0; i < 0xFF; i += 8) {
			printk(KERN_INFO "jr3pci(%d): [0x%02X] 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X",
				   card, i,
				   (unsigned short)readData(JR3_RAWCHANNELS + i,     card),
				   (unsigned short)readData(JR3_RAWCHANNELS + i + 1, card),
				   (unsigned short)readData(JR3_RAWCHANNELS + i + 2, card),
				   (unsigned short)readData(JR3_RAWCHANNELS + i + 3, card),
				   (unsigned short)readData(JR3_RAWCHANNELS + i + 4, card),
				   (unsigned short)readData(JR3_RAWCHANNELS + i + 5, card),
				   (unsigned short)readData(JR3_RAWCHANNELS + i + 6, card),
				   (unsigned short)readData(JR3_RAWCHANNELS + i + 7, card)
				   );
		}
	}

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
int jr3_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
long jr3_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	int err=0;
	int ret=0;
	int size = _IOC_SIZE(cmd); /* the size bitfield in cmd */
	int type = _IOC_TYPE(cmd);
	int nr = _IOC_NR(cmd);

	if ( type != JR3_IOC_MAGIC) return -ENOTTY;
	if ( nr > IOCTL_JR3_MAXNR) return -ENOTTY;

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0)
	  if (_IOC_DIR(cmd) & _IOC_READ)
		      err = !access_ok((void *)arg, size);

	  if (_IOC_DIR(cmd) & _IOC_WRITE)
	              err =  !access_ok((void *)arg, size);

	  if (err) return -EFAULT;
#else
	  if (_IOC_DIR(cmd) & _IOC_READ)
		      err = !access_ok(VERIFY_WRITE, (void *)arg, size);

	  if (_IOC_DIR(cmd) & _IOC_WRITE)
	              err =  !access_ok(VERIFY_READ, (void *)arg, size);

	  if (err) return -EFAULT;
#endif
	switch(cmd) {

		case IOCTL0_JR3_RESET:
			ret = jr3Reset(0);
			break;
		case IOCTL0_JR3_ZEROOFFS:
			ret = jr3ZeroOffs(0);
			break;
		case IOCTL0_JR3_FILTER0:
			ret = jr3Filter(arg, 0,0);
			break;
		case IOCTL0_JR3_FILTER1:
			ret = jr3Filter(arg, 1,0);
			break;
		case IOCTL0_JR3_FILTER2:
			ret = jr3Filter(arg, 2,0);
			break;
		case IOCTL0_JR3_FILTER3:
			ret = jr3Filter(arg, 3,0);
			break;
		case IOCTL0_JR3_FILTER4:
			ret = jr3Filter(arg, 4,0);
			break;
		case IOCTL0_JR3_FILTER5:
			ret = jr3Filter(arg, 5,0);
			break;
		case IOCTL0_JR3_FILTER6:
			ret = jr3Filter(arg, 6,0);
			break;
		case IOCTL0_JR3_GET_FULL_SCALES:
			ret = jr3GetFullScales(arg,0);
			break;
		case IOCTL0_JR3_SET_FULL_SCALES:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3SetFullScales(arg,0);
			else ret=-1;
			break;
		case IOCTL0_JR3_GET_FORCE_AND_RAW:
			ret = jr3GetForceAndRaw(arg);
			break;

		case IOCTL1_JR3_RESET:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Reset(1);
			else ret=-1;
			break;
		case IOCTL1_JR3_ZEROOFFS:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3ZeroOffs(1);
			else ret=-1;
			break;
		case IOCTL1_JR3_FILTER0:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Filter(arg, 0,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_FILTER1:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Filter(arg, 1,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_FILTER2:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Filter(arg, 2,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_FILTER3:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Filter(arg, 3,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_FILTER4:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Filter(arg, 4,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_FILTER5:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Filter(arg, 5,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_FILTER6:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3Filter(arg, 6,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_GET_FULL_SCALES:
			if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
				ret = jr3GetFullScales(arg,1);
			else ret=-1;
			break;
		case IOCTL1_JR3_SET_FULL_SCALES:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3SetFullScales(arg,1);
			else ret=-1;
			break;

		case IOCTL2_JR3_RESET:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Reset(2);
			else ret=-1;
			break;
		case IOCTL2_JR3_ZEROOFFS:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3ZeroOffs(2);
			else ret=-1;
			break;
		case IOCTL2_JR3_FILTER0:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 0,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_FILTER1:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 1,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_FILTER2:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 2,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_FILTER3:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 3,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_FILTER4:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 4,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_FILTER5:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 5,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_FILTER6:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 6,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_GET_FULL_SCALES:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3GetFullScales(arg,2);
			else ret=-1;
			break;
		case IOCTL2_JR3_SET_FULL_SCALES:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3SetFullScales(arg,2);
			else ret=-1;
			break;

		case IOCTL3_JR3_RESET:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Reset(3);
			else ret=-1;
			break;
		case IOCTL3_JR3_ZEROOFFS:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3ZeroOffs(3);
			else ret=-1;
			break;
		case IOCTL3_JR3_FILTER0:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 0,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_FILTER1:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 1,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_FILTER2:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 2,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_FILTER3:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 3,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_FILTER4:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 4,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_FILTER5:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 5,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_FILTER6:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3Filter(arg, 6,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_GET_FULL_SCALES:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3GetFullScales(arg,3);
			else ret=-1;
			break;
		case IOCTL3_JR3_SET_FULL_SCALES:
			if (PCI_DEVICE_ID_JR3==0x3114)
				ret = jr3SetFullScales(arg,3);
			else ret=-1;
			break;

		case IOCTL_JR3_DUMP_DSP:
			ret = jr3DumpDsp();
			break;

		default:
			return -ENOTTY;
	}
	return ret;
}
