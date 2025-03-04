using System;
using System.Runtime.InteropServices;
using System.Security;

namespace Maestro
{
    [SuppressUnmanagedCodeSecurity]
    public class MaestroSessionManager
    {
        public enum WhichHand
        {
            LeftHand,
            RightHand
        }

        /* not implemented in this version
        public enum OtaStatus
        {
            ota_error, //general unspecified error
            ota_pass, //success
            ota_checksumFail, //checksum mismatch
            ota_versionFail, //supplied version older than current version
            ota_noSpaceFail //no space on device for OTA files
        };
        */
        private const string DllName = "libccic";
        private const int StatusSuccess = 0;
        private const int StatusNotConnected = 1;
        private const int StatusNoDevice = 2;

        [DllImport(DllName)]
        private static extern int cci_create_mutable_haptic_session(out IntPtr sessionHandle);

        [DllImport(DllName)]
        private static extern void cci_close_session(IntPtr sessionHandle);

        [DllImport(DllName)]
        private static extern int cci_is_session_connected(IntPtr sessionHandle);

        [DllImport(DllName)]
        private static extern int cci_get_global_haptic_state(IntPtr sessionHandle, out IntPtr left, out IntPtr right);

        [DllImport(DllName)]
        private static extern int cci_get_session_haptic_state(IntPtr sessionHandle, out IntPtr left, out IntPtr right);

        [DllImport(DllName)]
        private static extern int cci_wait_global_haptic_state_changed(IntPtr sessionHandle, int timeoutMs, out bool stateChanged);

        [DllImport(DllName)]
        private static extern int cci_signal_session_haptic_state_changed(IntPtr sessionHandle);

        // These functions accept `null` for the second arg,
        // in which case it will just return true/false if a left/right is connected
        [DllImport(DllName)]
        private static extern int cci_get_left_device(IntPtr sessionHandle, IntPtr deviceDescriptionOut);

        [DllImport(DllName)]
        private static extern int cci_get_right_device(IntPtr sessionHandle, IntPtr deviceDescriptionOut);

        [DllImport(DllName)]
        private static extern IntPtr cci_get_error_string(int errorCode);

        /* not implemented in this version

        public delegate void OtaProgressCallback(float progress);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
        private static extern int cci_submit_ota_update(IntPtr sessionHandle, IntPtr deviceDescription, [MarshalAs(UnmanagedType.LPStr)] string uri, OtaProgressCallback callback,
            out OtaStatus status);
        */

        public IntPtr? SessionHandle { get; private set; }
        private bool WarningIssued = false;

        public event Action HapticStateChanged;
        private readonly CancellationTokenSource watcherSource = new();

        public bool StartSession()
        {
            if (SessionHandle.HasValue)
                return true;

            int result = cci_create_mutable_haptic_session(out IntPtr tempHandle);

            if (result != StatusSuccess)
            {
                if (!WarningIssued)
                {
                    Console.WriteLine($"Failed to create Contact CI service session: {GetErrorString(result)}");
                    WarningIssued = true;
                }

                return false;
            }
            else
            {
                WarningIssued = false;
            }

            SessionHandle = tempHandle;

            Console.WriteLine("Contact CI service session created.");
            return true;
        }

        public void CloseSession()
        {
            StopHapticWatcher();
            if (!SessionHandle.HasValue)
                return;

            cci_close_session(SessionHandle.Value);
            SessionHandle = null;
            Console.WriteLine("Contact CI service session closed.");
        }

        public void StartHapticWatcher()
        {
            //TODO reset watcherSource if it was stopped?
            Task.Run(()=>WatchState(watcherSource.Token), watcherSource.Token);
        }

        public void StopHapticWatcher()
        {
            watcherSource.Cancel();
        }

        private void WatchState(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                if (!SessionHandle.HasValue)
                {
                    Thread.Sleep(200);
                    continue;
                }

                cci_wait_global_haptic_state_changed(SessionHandle.Value, 1000, out bool change);
                if(change)
                    HapticStateChanged?.Invoke();
            }
        }

        public bool IsConnected()
        {
            if (!SessionHandle.HasValue)
                return false;

            int result = cci_is_session_connected(SessionHandle.Value);

            switch (result)
            {
                case StatusSuccess:
                    return true;
                case StatusNotConnected:
                    return false;
                default:
                    Console.WriteLine($"Failed check if Contact CI session is connected: {GetErrorString(result)}");
                    return false;
            }
        }

        public bool IsLeftDeviceConnected() => IsDeviceConnected(WhichHand.LeftHand);

        public bool IsRightDeviceConnected() => IsDeviceConnected(WhichHand.RightHand);

        private void GetDevice(WhichHand hand, out DeviceDescription device, out IntPtr ptr)
        {
            IntPtr ret = Marshal.AllocHGlobal(Marshal.SizeOf<DeviceDescription>());
            if (hand == WhichHand.LeftHand)
                cci_get_left_device(SessionHandle.Value, ret);
            else
                cci_get_right_device(SessionHandle.Value, ret);
            device = Marshal.PtrToStructure<DeviceDescription>(ret);
            ptr = ret;
        }

        public bool IsDeviceConnected(WhichHand whichHand)
        {
            if (!SessionHandle.HasValue)
                return false;

            int result = whichHand == WhichHand.LeftHand
                ? cci_get_left_device(SessionHandle.Value, IntPtr.Zero)
                : cci_get_right_device(SessionHandle.Value, IntPtr.Zero);

            switch (result)
            {
                case StatusSuccess:
                    return true;
                case StatusNoDevice:
                    return false;
                default:
                    Console.WriteLine($"Failed check if Contact CI device is connected: {GetErrorString(result)}");
                    return false;
            }
        }

        private string GetErrorString(int errorCode)
        {
            IntPtr strPtr = cci_get_error_string(errorCode);
            return Marshal.PtrToStringAnsi(strPtr);
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DeviceDescription
        {
            [MarshalAs(UnmanagedType.LPStr)]string productLine;
            [MarshalAs(UnmanagedType.LPStr)]string serialNumber;
            bool isRight;
            bool isConnected;
        }

        /* not implemented in this version
        public OtaStatus SubmitOtaUpdate(WhichHand hand, Uri source, OtaProgressCallback callback)
        {
            string uri = source.OriginalString;
            GetDevice(hand, out var _, out var ptr);
            Console.WriteLine($"passing uri {uri} ptr {ptr} sptr {SessionHandle.Value}");
            cci_submit_ota_update(SessionHandle.Value, ptr, uri, callback, out var status);
            return status;
        }
        */

        public void GetHaptics(out SerializableHapticState? left, out SerializableHapticState? right)
        {
            unsafe
            {
                int result = cci_get_global_haptic_state(SessionHandle.Value, out IntPtr rawLeft, out IntPtr rawRight);
                if (result != StatusSuccess)
                {
                    Console.WriteLine($"Failed to get Contact CI haptic state: {GetErrorString(result)}");
                    left = null;
                    right = null;
                    return;
                }

                var leftState = (SerializableHapticState*)rawLeft.ToPointer();
                var rightState = (SerializableHapticState*)rawRight.ToPointer();

                left = *leftState;
                right = *rightState;
            }
        }

        public unsafe void SetHaptics(WhichHand whichHand, ref SerializableHapticState state)
        {
            if (!SessionHandle.HasValue)
            {
                Console.WriteLine("NO SESSION!");
                return;
            }

            if (!IsConnected())
            {
                Console.WriteLine("SERVICE DISCONNECTED!");
                throw new Exception();
            }

            int result = cci_get_session_haptic_state(SessionHandle.Value, out IntPtr rawLeftState, out IntPtr rawRightState);

            if (result != StatusSuccess)
            {
                Console.WriteLine($"Failed to get Contact CI haptic state: {GetErrorString(result)}");
                return;
            }

            SerializableHapticState* hapticState = (SerializableHapticState*)(whichHand == WhichHand.LeftHand ? rawLeftState : rawRightState).ToPointer();

            hapticState->thumbForceFeedbackAmplitude = state.thumbForceFeedbackAmplitude;
            hapticState->indexForceFeedbackAmplitude = state.indexForceFeedbackAmplitude;
            hapticState->middleForceFeedbackAmplitude = state.middleForceFeedbackAmplitude;
            hapticState->ringForceFeedbackAmplitude = state.ringForceFeedbackAmplitude;
            hapticState->littleForceFeedbackAmplitude = state.littleForceFeedbackAmplitude;

            hapticState->thumbVibrationEffect = state.thumbVibrationEffect;
            hapticState->thumbVibrationModifier = state.thumbVibrationModifier;
            hapticState->thumbVibrationAmplitude = state.thumbVibrationAmplitude;

            hapticState->indexVibrationEffect = state.indexVibrationEffect;
            hapticState->indexVibrationModifier = state.indexVibrationModifier;
            hapticState->indexVibrationAmplitude = state.indexVibrationAmplitude;

            hapticState->middleVibrationEffect = state.middleVibrationEffect;
            hapticState->middleVibrationModifier = state.middleVibrationModifier;
            hapticState->middleVibrationAmplitude = state.middleVibrationAmplitude;

            hapticState->ringVibrationEffect = state.ringVibrationEffect;
            hapticState->ringVibrationModifier = state.ringVibrationModifier;
            hapticState->ringVibrationAmplitude = state.ringVibrationAmplitude;

            hapticState->littleVibrationEffect = state.littleVibrationEffect;
            hapticState->littleVibrationModifier = state.littleVibrationModifier;
            hapticState->littleVibrationAmplitude = state.littleVibrationAmplitude;


            result = cci_signal_session_haptic_state_changed(SessionHandle.Value);

            if (result != StatusSuccess)
            {
                Console.WriteLine($"Failed to raise Contact CI haptic state changed event: {GetErrorString(result)}");
                return;
            }
        }
    }
}