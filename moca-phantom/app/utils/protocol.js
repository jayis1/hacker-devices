// MoCA Phantom protocol helpers
// Author: jayis1

export const buildProfileCommand = (profile) => {
  const commands = {
    survey: { mode: 'survey', radio: true, inject: false, note: 'Authorized passive survey only' },
    riserAudit: { mode: 'inline', radio: true, inject: false, note: 'Authorized riser audit with leakage mapping' },
    activeValidation: { mode: 'inject', radio: true, inject: true, note: 'Authorized bounded manipulation mode' },
  };

  return commands[profile] || commands.survey;
};

export const summarizeNodeRisk = (node) => {
  if (!node.privacyEnabled) {
    return 'Privacy disabled';
  }
  if (node.rssi < -65) {
    return 'Weak path / possible leakage';
  }
  if (node.label.includes('adjacent')) {
    return 'Unexpected neighbor exposure';
  }
  return 'Nominal';
};
