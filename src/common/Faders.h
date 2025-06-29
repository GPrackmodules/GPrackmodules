#pragma once

#define FADER_STEPS		768
#define FADER_STEPS_F	(768.f)
#define FADER_ZERO_DB	(578.0f)

#define	MIN_PANBAL			-100
#define	MAX_PANBAL			100

#define SEND_STEPS			256

struct FaderGainQuantity : public ParamQuantity
{
	FaderGainQuantity();
	static float GainFactor(float fParam);
	std::string getDisplayValueString() override;
	void setDisplayValueString(std::string s) override;

private:
	static float FaderParam2dB(float fParam);	// parameter from 0.0 to 1.0
	static float FaderdB2Param(float fdB);
};

struct PanBalQuantity : public ParamQuantity
{
	PanBalQuantity();
	static float GainFactorL(float fParam, bool bIsBalance);
	static float GainFactorR(float fParam, bool bIsBalance);
	std::string getDisplayValueString() override;

private:
	static int ParamToIndex(float fParam);
};

struct SendQuantity : public ParamQuantity
{
	SendQuantity();
	static float GainFactor(float fParam);
	std::string getDisplayValueString() override;
	void setDisplayValueString(std::string s) override;
};


class GPaudioFader : public SvgSlider
{
protected:
	enum class FaderLength : int
	{
		Fader32mm = 32,
		Fader38mm = 38,
		Fader44mm = 44
	};

public:
	GPaudioFader(FaderLength eFaderLength);
	void SetFaderLength(FaderLength nMillimeter);
	void UpdateDarkMode();
	static float GainFactor(float fParam);

protected:
	void drawLayer(const widget::Widget:: DrawArgs &args, int nLayer) override;

private:
	bool m_bDarkMode;
	int m_nFaderLength;
	shared_ptr<Svg> m_pLightBackgroundSvg;
	shared_ptr<Svg> m_pDarkBackgroundSvg;
};

struct GPaudioSlider32mm : public GPaudioFader
{
	GPaudioSlider32mm() : GPaudioFader(GPaudioFader::FaderLength::Fader32mm) {}
};

struct GPaudioSlider38mm : public GPaudioFader
{
	GPaudioSlider38mm() : GPaudioFader(GPaudioFader::FaderLength::Fader38mm) {}
};

struct GPaudioSlider44mm : public GPaudioFader
{
	GPaudioSlider44mm() : GPaudioFader(GPaudioFader::FaderLength::Fader44mm) {}
};

