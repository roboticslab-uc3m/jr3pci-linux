/***************************************************************************
 *   Copyright (C) 2005 by Mario Prats                                     *
 *   mprats@icc.uji.es                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#define __MODULE__
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
#include <linux/config.h>
#endif
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#define __KERNEL_SYSCALLS__
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <asm/uaccess.h>
//#include <stdio.h>

#include "jr3pci-driver.h"
#include "jr3pci-firmware.h"
#include "jr3pci-ioctl.c"


#define JR3_DESC	"JR3 PCI force/torque sensor kernel module"
#define JR3_AUTHOR	"Mario Prats (mprats@icc.uji.es)"
#define JR3_MAJOR	39

MODULE_DESCRIPTION(JR3_DESC);
MODULE_AUTHOR(JR3_AUTHOR);
MODULE_LICENSE("GPL");
//MODULE_VERSION("0.1");

unsigned long memregion;
int size;

struct file_operations jr3_fops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
ioctl:		jr3_ioctl,
#else
unlocked_ioctl:	jr3_ioctl,
#endif
open:		jr3_open,
release:	jr3_release,
};

//static int errno;

static int show_copyright(short card) {
	int i;
	char copyright[19];
	char units_str[16];
	int base=0x6040;
	short day, year, units;
	
	for (i=0;i<18;i++) {
		copyright[i]=(char)(readData(base,card) >> 8);
		base++;
	}
	copyright[18]='\0';
	day=readData(0x60f6,card);
	year=readData(0x60f7,card);
	units=readData(0x60fc,card);
	if (units==0) {
		strncpy(units_str,"lbs\0",4);
	} else if (units==1) {
		strncpy(units_str,"Newtons\0",8);
	} else if (units==2) {
		strncpy(units_str,"Kilograms-force\0",16);
	} else if (units==3) {
		strncpy(units_str,"1000 lbs\0",8);
	} else {
		sprintf(units_str,"Unknown (id %d)",units);
	}
	printk(KERN_INFO "jr3pci(%d): %s\n",card,copyright);
	printk(KERN_INFO "jr3pci(%d): DSP Software updated day %d, year %d. Units: %s\n",card,day,year,units_str);
	return 0;
}

/** Download code to DSP */
static int jr3pci_initDSP(int card) {
	int i=0;
	int count,address,value, value2;
	
	count=jr3_firmware[i++];
	while (count != 0xffff) {
		address=jr3_firmware[i++];
		printk(KERN_INFO "jr3pci(%d): DSP Code. File pos. %d, Address 0x%x, %d times\n", card, i, address,count);
		
		while (count>0) {
			if (address & JR3_PD_DEVIDER) {
				value=jr3_firmware[i++];
				writeData(address,value,card);
				if (value!=readData(address,card)) {
					printk(KERN_INFO "data write error at address 0x%x", address);
				}
				count--;	
			} else {
				value=jr3_firmware[i++];
				value2=jr3_firmware[i++];
				writeProgram(address,value,value2,card);
				if ( ((value << 8) | (value2 & 0x00ff)) != readProgram(address, card) ) {
					printk(KERN_INFO "program write error at address 0x%x", address);
				}
				count-=2;
			}
			address++;
		}
		count=jr3_firmware[i++];
	}
	return 0;
}

/** Where is the card??? */
int jr3pci_probe(void) {
	int result=1;
	struct pci_dev *pci=NULL;
	pci=pci_get_device(PCI_VENDOR_ID_JR3, PCI_DEVICE_ID_JR3,pci);
	if (pci)
		if (!pci_enable_device(pci)) {
				memregion = pci_resource_start(pci, 0);
				size = pci_resource_len(pci,0);
				if (!check_mem_region(memregion,size))
				  if (request_mem_region(memregion,size,"JR3pci")) {
					jr3_base_address=ioremap(memregion,size);
					printk(KERN_INFO "jr3pci: mem mapped succesfully\n");
					result=0;
				  }
		}

	return result;
}

int __init jr3pci_init_module(void)
{
	int result;
	
	printk( KERN_INFO "jr3pci: %s by %s\n",JR3_DESC, JR3_AUTHOR );
	result=register_chrdev(JR3_MAJOR,"jr3pci",&jr3_fops);
	if (result<0) {
		printk(KERN_INFO "jr3pci: Can't open device file with major %d. Make sure it exists!\n", JR3_MAJOR);
		return result;
	}
	
	if (jr3pci_probe()) {
		printk( KERN_INFO "jr3pci: No devices found\n");
		unregister_chrdev(JR3_MAJOR,"jr3pci");
		return -ENODEV;
	}

	printk( KERN_INFO "jr3pci: JR3 PCI card detected at 0x%x\n", (int)jr3_base_address);
	

	
	//Reset DSP
	writeData(JR3_RESET_ADDRESS,0,0);
	if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
    {
		writeData(JR3_RESET_ADDRESS,0,1);
    }
	if (PCI_DEVICE_ID_JR3==0x3114)
    {
		writeData(JR3_RESET_ADDRESS,0,2);
		writeData(JR3_RESET_ADDRESS,0,3);
    }
	
	//Download DSP code
	jr3pci_initDSP(0);
	if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
    {
		jr3pci_initDSP(1);
    }
	if (PCI_DEVICE_ID_JR3==0x3114)
    {
		jr3pci_initDSP(2);
		jr3pci_initDSP(3);
    }
	
	show_copyright(0);
	if ((PCI_DEVICE_ID_JR3==0x3112)||(PCI_DEVICE_ID_JR3==0x3114))
    {
		show_copyright(1);
    }
	if (PCI_DEVICE_ID_JR3==0x3114)
    {
		show_copyright(2);
		show_copyright(3);
    }
	
	
	printk( KERN_INFO "jr3pci: DSP code downloaded!! You can start playing  :)\n");

	return 0;
}

void __init jr3pci_exit_module(void)
{
	iounmap(jr3_base_address);
	release_mem_region(memregion,size);
	unregister_chrdev(JR3_MAJOR,"jr3pci");
	printk( KERN_INFO "jr3pci: Module unloaded\n" );
}

module_init(jr3pci_init_module);
module_exit(jr3pci_exit_module);
