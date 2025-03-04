using System.Runtime.InteropServices;

namespace Maestro;

[StructLayout(LayoutKind.Sequential)]
public struct SerializableHapticState
{
    public float thumbForceFeedbackAmplitude;
    public float indexForceFeedbackAmplitude;
    public float middleForceFeedbackAmplitude;
    public float ringForceFeedbackAmplitude;
    public float littleForceFeedbackAmplitude;

    public float thumbForceFeedbackPosition;
    public float indexForceFeedbackPosition;
    public float middleForceFeedbackPosition;
    public float ringForceFeedbackPosition;
    public float littleForceFeedbackPosition;

    public uint thumbVibrationAmplitude;
    public uint indexVibrationAmplitude;
    public uint middleVibrationAmplitude;
    public uint ringVibrationAmplitude;
    public uint littleVibrationAmplitude;

    public byte thumbVibrationEffect;
    public byte indexVibrationEffect;
    public byte middleVibrationEffect;
    public byte ringVibrationEffect;
    public byte littleVibrationEffect;

    public byte thumbVibrationModifier;
    public byte indexVibrationModifier;
    public byte middleVibrationModifier;
    public byte ringVibrationModifier;
    public byte littleVibrationModifier;
}