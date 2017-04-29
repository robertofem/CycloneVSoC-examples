# CycloneVSoC_Examples
Examples using the FPSoC chip Cyclone V SoC

Repository folder map:
  --README.md: this file

  --DE1-SoC: Examples for Terasics DE1-SoC board

    --Baremetal:

    --Linux_Applications:

    --Linux_Modules: Linux Loadable Kernel Modules (LKM). Most of them come with test apps.
      --DMA_PL330_LKM: module to make transfers between application and FPGA using PLL330 DMAC.

    --Useful_Scripts: Linux shell scripts to ease configuration (fixed IP, DHCP, etc.).
      --fixed_mac_dhcp.sh: fixes MAC and asks IP using DHCP protocol.
