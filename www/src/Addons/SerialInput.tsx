import { useTranslation } from 'react-i18next';
import { FormCheck, Alert } from 'react-bootstrap';
import * as yup from 'yup';

import Section from '../Components/Section';
import { AddonPropTypes } from '../Pages/AddonsConfigPage';

export const serialInputScheme = {
	SerialInputAddonEnabled: yup
		.number()
		.required()
		.label('Serial Input Add-On Enabled'),
};

export const serialInputState = {
	SerialInputAddonEnabled: 0,
};

const SerialInput = ({ values, errors, handleChange, handleCheckbox }: AddonPropTypes) => {
	const { t } = useTranslation();
	
	return (
		<Section 
			title={t('AddonsConfig:serial-input-header-text')}
		>
			<div
				id="SerialInputAddonOptions"
				hidden={!values.SerialInputAddonEnabled}
			>
				<Alert variant="info">
					<Alert.Heading>{t('AddonsConfig:serial-input-sub-header')}</Alert.Heading>
					<p className="mb-2">
						{t('AddonsConfig:serial-input-description-1')}
					</p>
					<p className="mb-2">
						{t('AddonsConfig:serial-input-description-2')}
					</p>
					<hr />
					<p className="mb-0">
						<strong>{t('AddonsConfig:serial-input-protocol-label')}</strong>
					</p>
					<ul className="mb-2">
						<li>{t('AddonsConfig:serial-input-bit-0')}: A/B1</li>
						<li>{t('AddonsConfig:serial-input-bit-1')}: B/B2</li>
						<li>{t('AddonsConfig:serial-input-bit-2')}: X/B3</li>
						<li>{t('AddonsConfig:serial-input-bit-3')}: Y/B4</li>
						<li>{t('AddonsConfig:serial-input-bit-4')}: LB/L1</li>
						<li>{t('AddonsConfig:serial-input-bit-5')}: RB/R1</li>
						<li>{t('AddonsConfig:serial-input-bit-6')}: LT/L2</li>
						<li>{t('AddonsConfig:serial-input-bit-7')}: RT/R2</li>
						<li>{t('AddonsConfig:serial-input-bit-8')}: D-Up</li>
						<li>{t('AddonsConfig:serial-input-bit-9')}: D-Down</li>
						<li>{t('AddonsConfig:serial-input-bit-10')}: D-Left</li>
						<li>{t('AddonsConfig:serial-input-bit-11')}: D-Right</li>
						<li>{t('AddonsConfig:serial-input-bit-12')}: Select/S1</li>
						<li>{t('AddonsConfig:serial-input-bit-13')}: Start/S2</li>
						<li>{t('AddonsConfig:serial-input-bit-14')}: L3</li>
						<li>{t('AddonsConfig:serial-input-bit-15')}: R3</li>
					</ul>
					<p className="mb-0">
						<strong>{t('AddonsConfig:serial-input-example-label')}</strong>
					</p>
					<pre className="bg-dark text-light p-2 rounded">
						<code>
{`import serial
import struct

# Connect to GP2040-CE
ser = serial.Serial('/dev/ttyACM0', 115200)

# Press button A (bit 0)
button_mask = 1 << 0
ser.write(struct.pack('<I', button_mask))

# Release all buttons
ser.write(struct.pack('<I', 0))`}
						</code>
					</pre>
				</Alert>
			</div>
			<FormCheck
				label={t('Common:switch-enabled')}
				type="switch"
				id="SerialInputAddonButton"
				reverse
				isInvalid={false}
				checked={Boolean(values.SerialInputAddonEnabled)}
				onChange={(e) => {
					handleCheckbox('SerialInputAddonEnabled');
					handleChange(e);
				}}
			/>
		</Section>
	);
};

export default SerialInput;
