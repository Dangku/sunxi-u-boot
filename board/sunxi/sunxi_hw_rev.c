#include <common.h>
#include <asm/io.h>
#include <dm/uclass.h>
#include <dm/root.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <sunxi_board.h>
#include <sunxi_gpadc.h>

#define IS_RANGE(x, min, max)   ((x) > (min) && (x) < (max))

#define VOL_REF                               1800
#define RESOLUTION                       12

#define BOOTSEL_ADC_CH           0
#define DDRPARA1_ADC_CH       1
#define DDRPARA2_ADC_CH       2
#define BOARD_ADC_CH               3
#define BOM_ADC_CH                     4
#define DSI0_ADC_CH                     5
#define DSI1_ADC_CH                     6
#define EDP_ADC_CH                      7

//#include <stdio.h>

enum {
	BOOT_HDMI,
	BOOT_EDP_AML133X30,
	BOOT_DSI0_HC070IY30025,
	BOOT_DSI1_HC070IY30025
};

static int adc_to_voltage(int data) {
	return data * VOL_REF / (1 << RESOLUTION);
}

static int sunxi_enable_fdt_node(void *fdt_blob, char *name, int onoff)
{
	int nodeoffset = 0;
	int ret = 0;

	nodeoffset = fdt_path_offset(fdt_blob, name);
	ret = fdt_set_node_status(fdt_blob, nodeoffset,
		onoff ? FDT_STATUS_OKAY: FDT_STATUS_DISABLED, 0);

	if (ret < 0) {
		printf("set %s node status error\n", name);
	}

	return ret;
}

static int sunxi_update_de_fdt_node(void *fdt_blob)
{
	int nodeoffset = 0;
	int ret = 0;
	
	nodeoffset = fdt_path_offset(fdt_blob, "/soc/de");
	if (nodeoffset < 0) {
		printf("get /soc/de node fail\n");
		return -1;
	}

	ret = fdt_setprop_u32(fdt_blob, nodeoffset, "chn_cfg_mode", 3);
	if (ret < 0) {
		printf("update chn_cfg_mode to 3 fail\n");
	}

	return ret;
}

static int sunxi_update_fdt(void *fdt_blob, int boot_type)
{
	int ret;

	switch(boot_type) {
		case BOOT_EDP_AML133X30:
			sunxi_enable_fdt_node(fdt_blob, "/edp_backlight", 1);
			sunxi_enable_fdt_node(fdt_blob, "/edp_panel", 1);
			sunxi_enable_fdt_node(fdt_blob, "/soc/sunxi-drm/route/disp0_edp", 1);
			sunxi_enable_fdt_node(fdt_blob, "/soc/drm_edp", 1);
			sunxi_enable_fdt_node(fdt_blob, "/soc/tcon3", 1);
			sunxi_update_de_fdt_node(fdt_blob);
			break;
		case BOOT_DSI0_HC070IY30025:
			sunxi_update_de_fdt_node(fdt_blob);
			break;
		case BOOT_DSI1_HC070IY30025:
			sunxi_update_de_fdt_node(fdt_blob);
			break;
		case BOOT_HDMI:
		default:
			break;
	}

	/* scan again for bind device driver */
	ret = dm_scan_fdt(fdt_blob, false);
	if (ret) {
		printf("dm_scan_fdt() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

void sunxi_hw_rev(void)
{
	static int gpadc_flag;
	u32 bootsel_adc, ddrpara1_adc, ddrpara2_adc, board_adc, bom_adc, dsi0_adc, dsi1_adc, edp_adc;
	char s[128];

	if(!gpadc_flag) {
		sunxi_gpadc_init();
		gpadc_flag = 1;
	}

	bootsel_adc = sunxi_gpadc_read(BOOTSEL_ADC_CH);
	ddrpara1_adc = sunxi_gpadc_read(DDRPARA1_ADC_CH);
	ddrpara2_adc = sunxi_gpadc_read(DDRPARA2_ADC_CH);
	board_adc = sunxi_gpadc_read(BOARD_ADC_CH);
	bom_adc = sunxi_gpadc_read(BOM_ADC_CH);
	dsi0_adc = sunxi_gpadc_read(DSI0_ADC_CH);
	dsi1_adc = sunxi_gpadc_read(DSI1_ADC_CH);
	edp_adc = sunxi_gpadc_read(EDP_ADC_CH);

	printf("bootsel_adc=%d vol=%dmV\n", bootsel_adc, adc_to_voltage(bootsel_adc));
	printf("ddrpara1_adc=%d vol=%dmV\n", ddrpara1_adc, adc_to_voltage(ddrpara1_adc));
	printf("ddrpara2_adc=%d vol=%dmV\n", ddrpara2_adc, adc_to_voltage(ddrpara2_adc));
	printf("board_adc=%d vol=%dmV\n", board_adc, adc_to_voltage(board_adc));
	printf("bom_adc=%d vol=%dmV\n", bom_adc, adc_to_voltage(bom_adc));
	printf("dsi0_adc=%d vol=%dmV\n", dsi0_adc, adc_to_voltage(dsi0_adc));
	printf("dsi1_adc=%d vol=%dmV\n", dsi1_adc, adc_to_voltage(dsi1_adc));
	printf("edp_adc=%d vol=%dmV\n", edp_adc, adc_to_voltage(edp_adc));

	//if (IS_RANGE(dsi0_adc, 0, 4300)) {
	//	printf("dsi0 HC070IY30025 800x1280 panel connected\n");
	//	sunxi_update_fdt(BOOT_DSI0_HC070IY30025);
	//	snprintf(s, sizeof(s), "allwinner/sun55i-t527-bananapi-f5-%s.dtb", "dsi0-hc070iy30025");
	//}
	//if (IS_RANGE(dsi1_adc, 0, 4300)) {
	//	printf("dsi1 HC070IY30025 800x1280 panel connected\n");
	//	sunxi_update_fdt(BOOT_DSI1_HC070IY30025);
	//	snprintf(s, sizeof(s), "allwinner/sun55i-t527-bananapi-f5-%s.dtb", "dsi1-hc070iy30025");
	//}
	if (IS_RANGE(edp_adc, 0, 4300)) {
		tick_printf("edp  1920x1080 panel connected\n");
		sunxi_update_fdt((void *)gd->fdt_blob, BOOT_EDP_AML133X30);
		snprintf(s, sizeof(s), "allwinner/sun55i-t527-bananapi-f5-%s.dtb", "edp-aml133x30");
	} 
	//{
	//	printf("lcd not connected, hdmi only\n");
	//	sunxi_update_fdt(BOOT_HDMI);
	//	snprintf(s, sizeof(s), "allwinner/sun55i-t527-bananapi-f5.dtb");
	//}

	env_set("fdtfile", s);
	

}
