/* SPDX-License-Identifier: GPL-2.0-or-later
 * DWC2 full-speed, non-periodic descriptor DMA used by ESP-IDF's S3 HCD.
 * Included after the common DWC2 interrupt helpers. No firmware hooks.
 */
static void dwc2_descriptor_packet(DWC2State * s, uint32_t address, USBDevice * dev,
                                   USBEndpoint * ep, uint32_t index) {
  uint32_t channel = index >> 3;
  uint32_t hcchar = s->hreg1[index];
  uint32_t descriptor = s->hreg1[index + 5] & ~7u;
  uint32_t words[2], status, buffer, interrupt = 0;
  DWC2Packet * pending = &s->packet[channel];
  unsigned mps = get_field(hcchar, HCCHAR_MPS);
  unsigned type = get_field(hcchar, HCCHAR_EPTYPE);
  bool input = hcchar & HCCHAR_EPDIR;
  pending->devadr = address;
  pending->epnum = get_field(hcchar, HCCHAR_EPNUM);
  pending->epdir = input;
  pending->index = index;
  pending->needs_service = true;
  if (type != USB_ENDPOINT_XFER_CONTROL && type != USB_ENDPOINT_XFER_BULK) {
    interrupt = HCINTMSK_XACTERR;
    goto halt;
  }
  if (!mps || mps > 64 ||
      dma_memory_read(&s->dma_as, descriptor, words, sizeof(words), MEMTXATTRS_UNSPECIFIED) !=
          MEMTX_OK) {
    interrupt = HCINTMSK_AHBERR;
    goto halt;
  }
  status = le32_to_cpu(words[0]);
  buffer = le32_to_cpu(words[1]);
  if (!(status & (1u << 31))) {
    interrupt = 1u << 11; /* buffer not available */
    goto halt;
  }
  unsigned remaining = status & 0x1ffff;
  unsigned length = MIN(remaining, mps);
  unsigned token = !input && (status & (1u << 24)) ? USB_TOKEN_SETUP
                   : input                         ? USB_TOKEN_IN
                                                   : USB_TOKEN_OUT;
  pending->pid = token;
  if (!s->hreg1[index + 7])
    s->hreg1[index + 7] = buffer;
  uint32_t cursor = s->hreg1[index + 7];
  uint8_t bytes[64];
  if (token != USB_TOKEN_IN &&
      dma_memory_read(&s->dma_as, cursor, bytes, length, MEMTXATTRS_UNSPECIFIED) != MEMTX_OK) {
    interrupt = HCINTMSK_AHBERR;
    goto halt;
  }
  USBPacket packet;
  usb_packet_init(&packet);
  usb_packet_setup(&packet, token, ep, 0, descriptor, false, true);
  usb_packet_addbuf(&packet, bytes, length);
  usb_handle_packet(dev, &packet);
  int result = packet.status;
  unsigned actual = packet.actual_length;
  /* This token bridge is synchronous; reject unsupported asynchronous devices. */
  if (result == USB_RET_ASYNC)
    usb_cancel_packet(&packet);
  usb_packet_cleanup(&packet);
  if (result == USB_RET_NAK)
    return;
  if (result != USB_RET_SUCCESS || actual > length) {
    interrupt = result == USB_RET_STALL ? HCINTMSK_STALL : HCINTMSK_XACTERR;
    goto halt;
  }
  if (input &&
      dma_memory_write(&s->dma_as, cursor, bytes, actual, MEMTXATTRS_UNSPECIFIED) != MEMTX_OK) {
    interrupt = HCINTMSK_AHBERR;
    goto halt;
  }
  remaining -= actual;
  s->hreg1[index + 7] = cursor + actual;
  status = (status & ~0x1ffffu) | remaining;
  bool complete = !remaining || (input && actual < mps);
  if (complete)
    status &= ~(1u << 31);
  uint32_t written = cpu_to_le32(status);
  if (dma_memory_write(&s->dma_as, descriptor, &written, 4, MEMTXATTRS_UNSPECIFIED) != MEMTX_OK) {
    interrupt = HCINTMSK_AHBERR;
    goto halt;
  }
  if (!complete)
    return;
  if (status & (1u << 26)) {
    interrupt = HCINTMSK_XFERCOMPL;
    goto halt;
  }
  unsigned current = (descriptor >> 3) & 63;
  unsigned last = (s->hreg1[index + 4] >> 8) & 255;
  if (current >= last || current == 63) {
    interrupt = 1u << 11;
    goto halt;
  }
  s->hreg1[index + 5] = descriptor + 8;
  s->hreg1[index + 7] = 0;
  return;
halt:
  s->hreg1[index] &= ~HCCHAR_CHENA;
  s->hreg1[index + 2] |= interrupt | HCINTMSK_CHHLTD;
  pending->needs_service = false;
  dwc2_update_hc_irq(s, index);
}
