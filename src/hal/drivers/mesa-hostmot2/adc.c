
//
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#include <rtapi_slab.h>

#include "rtapi.h"
#include "rtapi_string.h"
#include "rtapi_math.h"

#include "hal.h"

#include "hal/drivers/mesa-hostmot2/hostmot2.h"




void hm2_adc_process_tram_read(hostmot2_t *hm2) {
    int adc;
    int chan;

    //
    // parse it out to the HAL pins
    //

    for (adc = 0; adc < hm2->adc.num_instances; adc ++) {
        for (chan = 0; chan < 8; chan++) {
            hal_u32_t val =  hm2->adc.chan_reg[chan];
            *hm2->adc.instance[chan].hal.pin.value = val;
        }
    }
}


int hm2_adc_parse_md(hostmot2_t *hm2, int md_index) {
    hm2_module_descriptor_t *md = &hm2->md[md_index];
    int r = 0;

    // 
    // some standard sanity checks
    //

    if (!hm2_md_is_consistent_or_complain(hm2, md_index, 0, 8, 4, 0)) {
	    HM2_ERR("inconsistent Module Descriptor!\n");
	    return -EINVAL;
    }

    if (hm2->adc.num_instances != 0) {
	    HM2_ERR(
			    "found duplicate Module Descriptor for %s (inconsistent firmware), not loading driver\n",
			    hm2_get_general_function_name(md->gtag)
		   );
	    return -EINVAL;
    }

    if (hm2->config.num_adcs > md->instances) {
	    HM2_ERR(
			    "config.num_adcs=%d, but only %d are available, not loading driver\n",
			    hm2->config.num_adcs,
			    md->instances
		   );
	    return -EINVAL;
    }

    if (hm2->config.num_adcs == 0) {
	    return 0;
    }


    // looks good, start initializing
    hm2->adc.chan_addr = md->base_address + (0 * md->register_stride);


   if (hm2->config.num_adcs == -1) {
      hm2->adc.num_instances = md->instances;
   } else {
      hm2->adc.num_instances = hm2->config.num_adcs;
   }


   hm2->adc.instance = (hm2_adc_instance_t *)hal_malloc(8 * sizeof(hm2_adc_instance_t));
    if (hm2->adc.instance == NULL) {
        HM2_ERR("out of memory!\n");
        r = -ENOMEM;
        goto fail0;
    }


    r = hm2_register_tram_read_region(hm2, hm2->adc.chan_addr, (8 * sizeof(rtapi_u32)), &hm2->adc.chan_reg);
    if (r < 0) {
	    HM2_ERR("error registering tram read region for ADC Register (%d)\n", r);
	    goto fail0;
    }

    {
	    int i;
	    int r;
	    char name[HAL_NAME_LEN + 1];

	    for (i = 0; i < 8; i ++) {
		    // pins
		    rtapi_snprintf(name, sizeof(name), "%s.adc.%02d.chan.%02d.value", hm2->llio->name, 0, i);
		    r = hal_pin_u32_new(name, HAL_IN, &(hm2->adc.instance[i].hal.pin.value), hm2->llio->comp_id);
		    if (r < 0) {
			    HM2_ERR("error adding pin '%s', aborting\n", name);
			    goto fail1;
		    }

		    /*
		    rtapi_snprintf(name, sizeof(name), "%s.adc.%02d.adc.%02d.enable", hm2->llio->name, 0, i);
		    r = hal_pin_bit_new(name, HAL_IN, &(hm2->adc.instance[i].hal.pin.enable), hm2->llio->comp_id);
		    if (r < 0) {
			    HM2_ERR("error adding pin '%s', aborting\n", name);
			    goto fail1;
		    }
		    */

		    // init hal objects
		    //*(hm2->adc.instance[i].hal.pin.enable) = 0;
		    *(hm2->adc.instance[i].hal.pin.value) = 0;
	    }
    }
    return hm2->adc.num_instances;

fail1:
    rtapi_kfree(hm2->adc.chan_reg);

fail0:
    hm2->adc.num_instances = 0;
    return r;
}


void hm2_adc_print_module(hostmot2_t *hm2) {
    int i;
    HM2_PRINT("ADC: %d\n", hm2->adc.num_instances);
    if (hm2->adc.num_instances <= 0) return;
    HM2_PRINT("    instances: 00%04d\n", hm2->adc.num_instances);
    for (i = 0; i < 8; i++ ) {
        HM2_PRINT("        chan %d:\n", i);
        HM2_PRINT("        pin value = 0x%04X\n", (*hm2->adc.instance[i].hal.pin.value));
    }
}


void hm2_adc_cleanup(hostmot2_t *hm2) {
    if (hm2->adc.num_instances <= 0) return;
    if (hm2->adc.chan_reg != NULL) rtapi_kfree(hm2->adc.chan_reg);
}


void hm2_adc_prepare_tram_write(hostmot2_t *hm2) {
}


void hm2_adc_force_write(hostmot2_t *hm2) {
}


// if the user has changed the adc config, sync it out
void hm2_adc_write(hostmot2_t *hm2) {
}
