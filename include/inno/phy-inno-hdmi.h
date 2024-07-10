/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __PHY_INNO_HDMI_H_
#define __PHY_INNO_HDMI_H_

/**
 * struct phy_configure_opts_inno_hdmi - INNO HDMI configuration set
 *
 * This structure is used to represent the configuration state of a
 * INNO HDMI phy.
 */
struct phy_configure_opts_inno_hdmi {
	/**
	 * @pixel_clock:
	 *
	 * pixel clock rate in Hertz
	 *
	 */
	unsigned int		pixel_clock;

	/**
	 * @bus_width:
	 *
	 * bus width to assign
	 */
	unsigned int		bus_width;

};

#endif /* __PHY_INNO_HDMI_H_ */
