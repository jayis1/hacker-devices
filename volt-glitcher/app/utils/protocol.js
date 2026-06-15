export const encodeGlitchConfig = (config) => {
    const buffer = new ArrayBuffer(12);
    const view = new DataView(buffer);
    view.setUint32(0, config.width, true);
    view.setUint32(4, config.delay, true);
    view.setUint16(8, config.triggerVal, true);
    view.setUint16(10, config.triggerMask, true);
    return buffer;
};

export const decodeStatus = (buffer) => {
    const view = new DataView(buffer);
    return {
        triggered: (view.getUint8(0) & 0x01) !== 0,
        complete: (view.getUint8(0) & 0x02) !== 0
    };
};
