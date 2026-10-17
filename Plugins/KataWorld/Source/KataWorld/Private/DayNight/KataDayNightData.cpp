#include "DayNight/KataDayNightData.h"

namespace
{
    struct FDefaultKey
    {
        float Hour;
        float Value;
    };

    /**
     * 커브에 기본 키를 넣는다. bFlatEnds면 첫 키와 마지막 키의 접선을 수평으로 고정한다.
     * 자정이 극값인 고도 커브는 0시와 24시의 기울기를 명시적으로 0에 고정해 자정 전후의 움직임을 부드럽게 잇는다.
     */
    void AddDefaultKeys(FRichCurve& RichCurve, std::initializer_list<FDefaultKey> Keys, ERichCurveInterpMode InterpMode, bool bFlatEnds = false)
    {
        for (const FDefaultKey& Key : Keys)
        {
            const FKeyHandle Handle = RichCurve.AddKey(Key.Hour, Key.Value);
            RichCurve.SetKeyInterpMode(Handle, InterpMode, false);
        }

        if (bFlatEnds && RichCurve.Keys.Num() > 0)
        {
            auto Flatten = [](FRichCurveKey& Key)
            {
                Key.TangentMode = RCTM_User;
                Key.ArriveTangent = 0.0f;
                Key.LeaveTangent = 0.0f;
            };
            Flatten(RichCurve.Keys[0]);
            Flatten(RichCurve.Keys.Last());
        }

        // 수동 접선(RCTM_User) 키는 건드리지 않고 나머지 Cubic 키의 접선만 계산한다.
        RichCurve.AutoSetTangents();
    }

    /** R·G·B 채널에 같은 선형 키를 넣어 무채색 배율 커브를 만든다. Alpha는 비워 두면 1로 평가된다. */
    void AddDefaultGreyKeys(FRuntimeCurveLinearColor& Curve, std::initializer_list<FDefaultKey> Keys)
    {
        for (int32 Channel = 0; Channel < 3; ++Channel)
        {
            AddDefaultKeys(Curve.ColorCurves[Channel], Keys, RCIM_Linear);
        }
    }
}

UKataDayNightData::UKataDayNightData()
{
    // 해는 5.5시에 떠서 12시에 가장 높고 18.5시에 진다. 6시와 18시에 해가 지평선 위 약 5도에 있어 비스듬한 노을빛이 지면에 닿는다.
    // 해가 정확히 지평선에 있으면 대기를 길게 지나며 직사광이 거의 사라져, 하늘만 밝고 지면은 역광 실루엣이 된다.
    // 달은 18시에 떠서 0시에 가장 높고 6시에 진다.
    // 세기는 떠 있는 동안 일정하게 둔다. 지평선 근처의 감쇠와 노을색은 Sky Atmosphere가 만들므로, 세기까지 미리 줄이면 해 질 녘이 한밤보다 어두워진다.
    // 광원이 지평선 아래 10~19도쯤 내려간 구간(해 4~4.5시·19.5~20시, 달 7~7.5시·16.5~17시)에서 세기를 0으로 내려, 박명이 끝난 뒤 보이지 않는 조명의 그림자 계산을 줄인다.
    AddDefaultKeys(Sun.Elevation.EditorCurveData, { { 0.0f, -60.0f }, { 5.5f, 0.0f }, { 12.0f, 60.0f }, { 18.5f, 0.0f }, { 24.0f, -60.0f } }, RCIM_Cubic, true);
    AddDefaultKeys(Sun.Azimuth.EditorCurveData, { { 0.0f, 0.0f }, { 24.0f, 360.0f } }, RCIM_Linear);
    AddDefaultKeys(Sun.Intensity.EditorCurveData, { { 4.0f, 0.0f }, { 4.5f, 10.0f }, { 19.5f, 10.0f }, { 20.0f, 0.0f } }, RCIM_Linear);

    AddDefaultKeys(Moon.Elevation.EditorCurveData, { { 0.0f, 50.0f }, { 6.0f, 0.0f }, { 12.0f, -50.0f }, { 18.0f, 0.0f }, { 24.0f, 50.0f } }, RCIM_Cubic, true);
    AddDefaultKeys(Moon.Azimuth.EditorCurveData, { { 0.0f, 180.0f }, { 24.0f, 540.0f } }, RCIM_Linear);
    AddDefaultKeys(Moon.Intensity.EditorCurveData, { { 0.0f, 0.3f }, { 7.0f, 0.3f }, { 7.5f, 0.0f }, { 16.5f, 0.0f }, { 17.0f, 0.3f }, { 24.0f, 0.3f } }, RCIM_Linear);

    // 노을 무렵에는 하늘이 밝은 데 비해 그늘이 어두워지므로 Sky Light를 2배로 올려 그늘을 하늘빛으로 채운다.
    AddDefaultKeys(SkyLightIntensity.EditorCurveData, { { 0.0f, 1.0f }, { 4.5f, 1.0f }, { 5.5f, 2.0f }, { 6.5f, 2.0f }, { 7.5f, 1.0f }, { 16.5f, 1.0f }, { 17.5f, 2.0f }, { 18.5f, 2.0f }, { 19.5f, 1.0f }, { 24.0f, 1.0f } }, RCIM_Linear);

    // 달빛도 하늘을 낮과 같은 파란색으로 산란시켜, 밤에는 하늘이 지면보다 밝고 채도가 높아 낮처럼 보인다. 박명이 끝난 뒤에만 하늘 휘도를 0.15로 낮춘다.
    AddDefaultGreyKeys(SkyLuminanceFactor, { { 0.0f, 0.15f }, { 4.0f, 0.15f }, { 5.0f, 1.0f }, { 19.0f, 1.0f }, { 20.0f, 0.15f }, { 24.0f, 0.15f } });
}
