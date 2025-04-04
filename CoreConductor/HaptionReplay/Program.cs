using System.Collections.Concurrent;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Text;
using Maestro;

namespace HaptionReplay;

class Program
{

    private static MaestroSessionManager _session;
    private static Stopwatch _stopwatch;

    static void Main(string[] args)
    {
        _session = new MaestroSessionManager();
        while (!_session.StartSession())
        {
            Console.WriteLine("Service not connected! Waiting.");
            Thread.Sleep(5000);
        }

        Console.WriteLine("Session open.");

        bool doAnalysis = false;
        string? path = null;
        if (args.Length == 0)
        {
            string? s = null;
            while (string.IsNullOrEmpty(s))
            {
                Console.WriteLine("Choose an option: ");
                Console.WriteLine("1: Record session\n" +
                                  "2: Play back recording\n" +
                                  "3: Analyze recording\n" +
                                  "4: Live analysis (with recording)\n" +
                                  "5: Live analysis (no recording)");


                s = Console.ReadLine();

                if(string.IsNullOrEmpty(s))
                {
                    Console.Clear();
                    Console.WriteLine("Error: input not understood.");
                    s = null;
                    continue;
                }

                if (s.Equals("1", StringComparison.CurrentCultureIgnoreCase))
                {
                    ListenHaptics(true, false);
                    ExitWait("Something happened during ListenHaptics! This should not happen!");
                }
                else if (s.Equals("2", StringComparison.CurrentCultureIgnoreCase))
                {
                    Console.WriteLine("input path:");
                    path = Console.ReadLine();
                    break;
                }
                else if (s.Equals("3", StringComparison.CurrentCultureIgnoreCase))
                {
                    Console.WriteLine("input path:");
                    path = Console.ReadLine();
                    doAnalysis = true;
                    break;
                }
                else if (s.Equals("4", StringComparison.CurrentCultureIgnoreCase))
                {
                    ListenHaptics(true, true);
                    ExitWait("Something happened during ListenHaptics! This should not happen!");
                }
                else if (s.Equals("5", StringComparison.CurrentCultureIgnoreCase))
                {
                    ListenHaptics(false, true);
                    ExitWait("Something happened during ListenHaptics! This should not happen!");
                }
                else
                {
                    Console.Clear();
                    Console.WriteLine("Error: input not understood.");
                    s = null;
                    continue;

                }
            }
        }
        else if (args.Length != 1)
        {
            ExitWait("Error. Expected one input parameter");
        }
        else
        {
            path = args[0];
        }

        Console.WriteLine($"Checking input file {path}");
        if (!File.Exists(path))
            ExitWait("File does not exist!");
        if (!Path.GetExtension(path).Equals(".hrv", StringComparison.CurrentCultureIgnoreCase))
            ExitWait("File is not a '.hrv' file!");

        Console.WriteLine("Loading sequence...");

        //deserialize the entire file into memory
        //this could be done during the playback loop, but we're looking to capture very fast transitions
        //and I don't want to risk IO and deserialization taking longer than the transition time
        var steps = new List<Tuple<double, SerializableHapticState?, SerializableHapticState?>>();
        double? offset = null;
        foreach (var line in File.ReadLines(path))
        {
            if (string.IsNullOrEmpty(line))
            {
                continue;
            }

            var splits = line.Split(',');
            var time = Double.Parse(splits[0]);
            if (offset == null)
                offset = time;
            time -= offset.Value;
            var sleft = Convert.FromBase64String(splits[1]);
            var sright = Convert.FromBase64String(splits[2]);
            SerializableHapticState? left = null;
            SerializableHapticState? right = null;
            if (sleft.Any())
                left = Deserialize(sleft);
            if (sright.Any())
                right = Deserialize(sright);
            steps.Add(new Tuple<double, SerializableHapticState?, SerializableHapticState?>(time, left, right));
        }

        Console.WriteLine($"Loaded {steps.Count} frames with a total DT of {(steps.Last().Item1 - steps[0].Item1) / 1000f:N2} seconds");

        if (doAnalysis)
        {
            Console.WriteLine("Beginning haptic analysis...");
            Analyze(steps, true);
            ExitWait("ok");
        }
        else
        {
            var nextStep = steps[0];
            int index = 0;
            _stopwatch = Stopwatch.StartNew();
            while (true)
            {
                var (time, left, right) = nextStep;
                if (_stopwatch.Elapsed.TotalMilliseconds < time)
                    continue;
                if (left.HasValue)
                {
                    var rleft = left.Value;
                    _session.SetHaptics(MaestroSessionManager.WhichHand.LeftHand, ref rleft);
                }

                if (right.HasValue)
                {
                    var rright = right.Value;
                    _session.SetHaptics(MaestroSessionManager.WhichHand.RightHand, ref rright);
                }

                Console.Write($"{index}/{steps.Count}          \r");
                index++;
                if (index >= steps.Count)
                {
                    index = 0;
                    _stopwatch.Restart();
                }

                nextStep = steps[index];
            }
        }
    }

    [DoesNotReturn]
    private static void ExitWait(string message, int exitCode = 1)
    {
        Console.WriteLine(message);
        Console.WriteLine("Press enter to exit");
        Console.ReadLine();
        Environment.Exit(exitCode);
    }

    private static void Analyze(List<Tuple<double, SerializableHapticState?, SerializableHapticState?>> steps, bool saveAnalysis)
    {
        StreamWriter? fs = null;
        TextWriter? oldOut = null;
        if(saveAnalysis)
        {
            fs = File.CreateText($"analysis-{DateTime.Now:yy-MM-dd_hh-mm}.txt");
            oldOut = Console.Out;
            Console.SetOut(fs);
        }
        bool first = true;
        int hpos = Console.BufferWidth / 2;
        Console.Write("Left:");
        Console.SetCursorPosition(hpos, Console.CursorTop);
        Console.WriteLine("Right:");
        for(int i = 0; i < Console.BufferWidth; i++)
            Console.Write('=');
        Console.WriteLine();
        SerializableHapticState? pLeft = null;
        SerializableHapticState? pRight = null;
        double pStep = 0;
        foreach (var step in steps)
        {
            var left = step.Item2;
            var right = step.Item3;
            foreach (var field in typeof(SerializableHapticState).GetFields(BindingFlags.Public | BindingFlags.Instance))
            {
                bool line = false;
                if (left != null)
                {
                    if (field.FieldType == typeof(float))
                    {
                        float pf = float.NegativeInfinity;
                        float f = (float)field.GetValue(left);
                        if (pLeft != null)
                            pf = (float)field.GetValue(pLeft);

                        if (!f.Equals(pf))
                        {
                            Console.Write($"{field.Name}: {f}");
                            line = true;
                        }

                    }
                    else if (field.FieldType == typeof(uint))
                    {
                        uint pu = uint.MaxValue;
                        uint u = (uint)field.GetValue(left);
                        if (pLeft != null)
                            pu = (uint)field.GetValue(pLeft);

                        if(!u.Equals(pu))
                        {
                            Console.Write($"{field.Name}: {u}");
                            if(u > 127)
                                Console.Write($" :: {u:X8}");
                            line = true;
                        }
                    }
                    else if (field.FieldType == typeof(byte))
                    {
                        byte pb = byte.MaxValue;
                        byte b = (byte)field.GetValue(left);
                        if (pLeft != null)
                            pb = (byte)field.GetValue(pLeft);

                        if(!b.Equals(pb))
                        {
                            Console.Write($"{field.Name}: {b}");
                            line = true;
                        }
                    }
                }

                if (right != null)
                {
                    Console.SetCursorPosition(hpos, Console.CursorTop);

                    if (field.FieldType == typeof(float))
                    {
                        float pf = float.NegativeInfinity;
                        float f = (float)field.GetValue(right);
                        if (pRight != null)
                            pf = (float)field.GetValue(pRight);


                        if (!f.Equals(pf))
                        {
                            Console.Write($"{field.Name}: {f}");
                            line = true;
                        }

                    }
                    else if (field.FieldType == typeof(uint))
                    {
                        uint pu = uint.MaxValue;
                        uint u = (uint)field.GetValue(right);
                        if (pRight != null)
                            pu = (uint)field.GetValue(pRight);

                        if(!u.Equals(pu))
                        {
                            Console.Write($"{field.Name}: {u}");
                            if(u > 127)
                                Console.Write($" :: {u:X8}");
                            line = true;
                        }
                    }
                    else if (field.FieldType == typeof(byte))
                    {
                        byte pb = byte.MaxValue;
                        byte b = (byte)field.GetValue(right);
                        if (pRight != null)
                            pb = (byte)field.GetValue(pRight);

                        if(!b.Equals(pb))
                        {
                            Console.Write($"{field.Name}: {b}");
                            line = true;
                        }
                    }
                }

                if (line)
                    Console.WriteLine();
            }

            Console.WriteLine($"DT: {step.Item1 - pStep}");
            pStep = step.Item1;

            if (left != null)
                pLeft = left;
            if (right != null)
                pRight = right;
        }

        if(saveAnalysis)
        {
            Console.SetOut(oldOut);
            fs.Close();
        }
    }

[DoesNotReturn]
    static void ListenHaptics(bool record, bool liveAnalyze)
    {
        Console.WriteLine("Starting haptic watcher.");
        _session.HapticStateChanged += SessionOnHapticStateChanged;
        _session.StartHapticWatcher();
        _stopwatch = Stopwatch.StartNew();

        Console.WriteLine("Ready");
        Console.WriteLine("Ctrl + C to exit");
        string path = $"HapticCapture-{DateTime.Now:yy-MM-dd_hh-mm}.hrv";
        while (true)
        {
            if (!_stateQueue.TryDequeue(out var line))
            {
                Thread.Sleep(10);
                continue;
            }

            var (time, left, right) = line;

            if (liveAnalyze)
            {
                var list = new List<Tuple<double, SerializableHapticState?, SerializableHapticState?>>();
                list.Add(new Tuple<double, SerializableHapticState?, SerializableHapticState?>(time.TotalMilliseconds, left, right));
                Analyze(list, false);
            }

            if (record)
            {
                var sb = new StringBuilder();
                var sleft = Array.Empty<byte>();
                var sright = Array.Empty<byte>();
                if (left.HasValue)
                {
                    sleft = Serialize(left.Value);
                }

                if (right.HasValue)
                {
                    sright = Serialize(right.Value);
                }

                sb.Append(time.TotalMilliseconds).Append(',');
                sb.Append(Convert.ToBase64String(sleft)).Append(',');
                sb.Append(Convert.ToBase64String(sright));

                using var f = File.AppendText(path);
                f.WriteLine(sb.ToString());
                f.Close();
            }

            if (liveAnalyze)
            {

            }
        }
    }

    private static ConcurrentQueue<(TimeSpan, SerializableHapticState?, SerializableHapticState?)> _stateQueue = new();

    private static void SessionOnHapticStateChanged()
    {
        var elapsed = _stopwatch.Elapsed;
        _session.GetHaptics(out var left, out var right);
        _stateQueue.Enqueue((elapsed, left, right));
    }

    static byte[] Serialize(SerializableHapticState state)
    {
        List<byte> ret = new();
        ret.AddRange(BitConverter.GetBytes(state.thumbForceFeedbackAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.indexForceFeedbackAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.middleForceFeedbackAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.ringForceFeedbackAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.littleForceFeedbackAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.thumbForceFeedbackPosition));
        ret.AddRange(BitConverter.GetBytes(state.indexForceFeedbackPosition));
        ret.AddRange(BitConverter.GetBytes(state.middleForceFeedbackPosition));
        ret.AddRange(BitConverter.GetBytes(state.ringForceFeedbackPosition));
        ret.AddRange(BitConverter.GetBytes(state.littleForceFeedbackPosition));
        ret.AddRange(BitConverter.GetBytes(state.thumbVibrationAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.indexVibrationAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.middleVibrationAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.ringVibrationAmplitude));
        ret.AddRange(BitConverter.GetBytes(state.littleVibrationAmplitude));
        ret.Add(state.thumbVibrationEffect);
        ret.Add(state.indexVibrationEffect);
        ret.Add(state.middleVibrationEffect);
        ret.Add(state.ringVibrationEffect);
        ret.Add(state.littleVibrationEffect);
        ret.Add(state.thumbVibrationModifier);
        ret.Add(state.indexVibrationModifier);
        ret.Add(state.middleVibrationModifier);
        ret.Add(state.ringVibrationModifier);
        ret.Add(state.littleVibrationModifier);
        return ret.ToArray();
    }

    static SerializableHapticState Deserialize(byte[] data)
    {
        int offset = 0;
        var ret = new SerializableHapticState();
        ret.thumbForceFeedbackAmplitude = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.indexForceFeedbackAmplitude = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.middleForceFeedbackAmplitude = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.ringForceFeedbackAmplitude = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.littleForceFeedbackAmplitude = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.thumbForceFeedbackPosition = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.indexForceFeedbackPosition = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.middleForceFeedbackPosition = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.ringForceFeedbackPosition = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.littleForceFeedbackPosition = BitConverter.ToSingle(data, offset);
        offset += sizeof(float);
        ret.thumbVibrationAmplitude = BitConverter.ToUInt32(data, offset);
        offset += sizeof(uint);
        ret.indexVibrationAmplitude = BitConverter.ToUInt32(data, offset);
        offset += sizeof(uint);
        ret.middleVibrationAmplitude = BitConverter.ToUInt32(data, offset);
        offset += sizeof(uint);
        ret.ringVibrationAmplitude = BitConverter.ToUInt32(data, offset);
        offset += sizeof(uint);
        ret.littleVibrationAmplitude = BitConverter.ToUInt32(data, offset);
        offset += sizeof(uint);
        ret.thumbVibrationEffect = data[offset++];
        ret.indexVibrationEffect = data[offset++];
        ret.middleVibrationEffect = data[offset++];
        ret.ringVibrationEffect = data[offset++];
        ret.littleVibrationEffect = data[offset++];
        ret.thumbVibrationModifier = data[offset++];
        ret.indexVibrationModifier = data[offset++];
        ret.middleVibrationModifier = data[offset++];
        ret.ringVibrationModifier = data[offset++];
        ret.littleVibrationModifier = data[offset++];
        return ret;
    }

}