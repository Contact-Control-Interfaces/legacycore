using System.Collections.Concurrent;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Linq.Expressions;
using System.Reflection;
using System.Text;
using Maestro;

namespace HaptionReplay;

class Program
{

    private static MaestroSessionManager _session;
    private static Stopwatch _stopwatch;

    private static bool stress;
    private static bool leftOnline;
    private static bool rightOnline;
    private static DateTime? leftStart;
    private static DateTime? leftEnd;
    private static DateTime? rightStart;
    private static DateTime? rightEnd;
    private static DateTime playbackStart;
    static void Main(string[] args)
    {
        if (Console.WindowWidth < 100)
        {
            Console.SetWindowSize(100, Console.WindowHeight);
            Console.BufferWidth = 100;
        }
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
                                  "5: Live analysis (no recording)\n" +
                                  "6: Stress test");


                s = Console.ReadLine();

                if(string.IsNullOrEmpty(s))
                {
                    Console.Clear();
                    Console.WriteLine("Error: input not understood.");
                    s = null;
                    continue;
                }

                if (s.Length > 1)
                {
                    Console.Clear();
                    Console.WriteLine("Error: input not understood.");
                    s = null;
                    continue;
                }

                if (!int.TryParse(s, out int val))
                {
                    Console.Clear();
                    Console.WriteLine("Error: input not understood.");
                    s = null;
                    continue;
                }

                if (val == 1)
                {
                    ListenHaptics(true, false);
                    ExitWait("Something happened during ListenHaptics! This should not happen!");
                }
                else if (val == 2)
                {
                    Console.WriteLine("input path:");
                    path = Console.ReadLine();
                    break;
                }
                else if (val == 3)
                {
                    Console.WriteLine("input path:");
                    path = Console.ReadLine();
                    doAnalysis = true;
                    break;
                }
                else if (val == 4)
                {
                    ListenHaptics(true, true);
                    ExitWait("Something happened during ListenHaptics! This should not happen!");
                }
                else if (val == 5)
                {
                    ListenHaptics(false, true);
                    ExitWait("Something happened during ListenHaptics! This should not happen!");
                }
                else if (val == 6)
                {
                    stress = true;
                    Console.WriteLine("input path:");
                    path = Console.ReadLine();
                    break;
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
        Console.Write("0%");

        //deserialize the entire file into memory
        //this could be done during the playback loop, but we're looking to capture very fast transitions
        //and I don't want to risk IO and deserialization taking longer than the transition time
        var steps = new List<Tuple<double, SerializableHapticState?, SerializableHapticState?>>();
        double? offset = null;
        var lines = File.ReadLines(path).ToList();
        for(var i = 0; i < lines.Count; i++)
        {
            var progress = (float)i / lines.Count;
            Console.CursorLeft = 0;
            Console.Write($"{(int)MathF.Ceiling(progress * 100)}%");
            var line = lines[i];
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
        Console.WriteLine();

        Console.WriteLine($"Loaded {steps.Count} frames with a total runtime of {(steps.Last().Item1 - steps[0].Item1) / 1000f:N2} seconds");

        if (doAnalysis)
        {
            Console.WriteLine("Beginning haptic analysis...");
            Analyze(steps, true, false, path.Replace("HapticCapture", "Analysis"));
            ExitWait("ok");
        }
        else
        {
            playbackStart = DateTime.Now;
            Console.WriteLine();
            var nextStep = steps[0];
            int index = 0;
            _stopwatch = Stopwatch.StartNew();
            while (true)
            {
                if (_session.IsLeftDeviceConnected())
                {
                    if (!leftOnline)
                    {
                        leftStart = DateTime.Now;
                        leftOnline = true;
                    }
                }
                else
                {
                    if (leftOnline)
                    {
                        leftEnd = DateTime.Now;
                        leftOnline = false;
                    }
                } 
                if (_session.IsRightDeviceConnected())
                {
                    if (!rightOnline)
                    {
                        rightStart = DateTime.Now;
                        rightOnline = true;
                    }
                }
                else
                {
                    if (rightOnline)
                    {
                        rightEnd = DateTime.Now;
                        rightOnline = false;
                    }
                }

                if (!leftOnline && !rightOnline)
                {
                    if(!leftStart.HasValue && !rightStart.HasValue)
                    {
                        Console.SetCursorPosition(0, Console.WindowHeight);
                        Console.Write("Waiting for glove(s) to connect...");
                        Thread.Sleep(250);
                        continue;
                    }
                    WriteStats();
                    Console.WriteLine();
                    Console.Write("Left elapsed: ");
                    if (leftStart.HasValue)
                    {
                        if(leftEnd.HasValue)
                            Console.Write($"{(leftEnd - leftStart):hh\\:mm\\:ss} ");
                        else
                            Console.Write($"{(DateTime.Now - leftStart):hh\\:mm\\:ss} (running) ");
                    }
                    else
                        Console.Write("NA ");
                    Console.Write("Right elapsed: ");
                    if (rightStart.HasValue)
                    {
                        if(rightEnd.HasValue)
                            Console.Write($"{rightEnd - rightStart:hh\\:mm\\:ss}");
                        else
                            Console.Write($"{DateTime.Now - rightStart:hh\\:mm\\:ss} (running)");
                    }
                    else
                        Console.Write("NA ");
                    if (stress)
                    {
                        string stressPath = $"StressResults-{playbackStart:yy-MM-dd_hh-mm}.txt";
                        var sb = new StringBuilder();
                        sb.AppendLine($"Playback start: {playbackStart:hh\\:mm\\:ss}");
                        sb.AppendLine($"Playback end: {DateTime.Now:hh\\:mm\\:ss}");
                        sb.AppendLine($"Playback runtime: {DateTime.Now - playbackStart:hh\\:mm\\:ss}");
                        sb.Append($"Left glove runtime: ");
                        sb.AppendLine(leftStart.HasValue ? $"{leftEnd - leftStart:hh\\:mm\\:ss}" : "NA");
                        sb.Append($"Right glove runtime: ");
                        sb.AppendLine(rightStart.HasValue ? $"{rightEnd - rightStart:hh\\:mm\\:ss}" : "NA");
                        File.WriteAllText(stressPath, sb.ToString());
                        Console.WriteLine($"Stress test results saved to {stressPath}");
                    }
                    ExitWait("All devices disconnected.");
                }

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

                //Console.Write($"{index}/{steps.Count}          \r");
                WriteStats($"{index}/{steps.Count} ");
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
        _session.CloseSession();
        Console.WriteLine(message);
        Console.WriteLine("Press enter to exit");
        Console.ReadLine();
        Environment.Exit(exitCode);
    }

    private static void WriteStats(string? prefix = null)
    {
        //Console.WriteLine();
        Console.SetCursorPosition(0,  Console.WindowHeight - 1);
        int currentLineCursor = Console.CursorTop;
        Console.SetCursorPosition(0, Console.CursorTop);
        Console.Write(new string(' ', Console.WindowWidth)); 
        Console.SetCursorPosition(0, currentLineCursor);
        Console.Write($"{prefix ?? ""}Left: {(leftOnline ? "ON" : "OFF")} Right: {(rightOnline ? "ON" : "OFF")} Elapsed: {DateTime.Now - playbackStart:hh\\:mm\\:ss}");
        Console.SetCursorPosition(0, Console.WindowHeight);
        currentLineCursor = Console.CursorTop;
        Console.SetCursorPosition(0, Console.CursorTop);
        Console.Write(new string(' ', Console.WindowWidth)); 
        Console.SetCursorPosition(0, currentLineCursor);
        Console.Write("Left elapsed: ");
        if (leftStart.HasValue)
        {
            if(leftEnd.HasValue)
                Console.Write($"{(leftEnd - leftStart):hh\\:mm\\:ss} ");
            else
                Console.Write($"{(DateTime.Now - leftStart):hh\\:mm\\:ss} (running) ");
        }
        else
            Console.Write("NA ");
        Console.Write("Right elapsed: ");
        if (rightStart.HasValue)
        {
            if(rightEnd.HasValue)
                Console.Write($"{rightEnd - rightStart:hh\\:mm\\:ss}");
            else
                Console.Write($"{DateTime.Now - rightStart:hh\\:mm\\:ss} (running)");
        }
        else
            Console.Write("NA ");

        //Console.SetCursorPosition(0, Console.CursorTop);
    }

    private static string padString(string message, int width)
    {
        if (message.Length > width)
            throw new ArgumentException("Supplied message is longer than fragment width!");
        return message + new string(' ', width - message.Length);
    }

    private static Dictionary<FieldInfo, Delegate> _stateAccessors = new();

    private static T? GetReflectionValue<T>(FieldInfo field, object? obj) where T : struct
    {
        if (obj == null)
            return null;

        if (!_stateAccessors.TryGetValue(field, out var accessor))
        {
            var param = Expression.Parameter(typeof(object), "obj");
            var castObj = Expression.Convert(param, field.DeclaringType!);
            var fieldAccess = Expression.Field(castObj, field);
            var lambda = Expression.Lambda<Func<object, T>>(fieldAccess, param);
            accessor = lambda.Compile();
            _stateAccessors.Add(field, accessor);
        }

        var del = (Func<object, T>)accessor;
        return del(obj);
    }

    private static string? ComputeLogValue<T>(FieldInfo field, object? left, object? prevLeft, object? right, object? prevRight, Func<T?, string>? formatter = null,
        int hOffset = 50)
        where T : struct
    {
        var valLeft = GetReflectionValue<T>(field, left);
        var prevValLeft = GetReflectionValue<T>(field, prevLeft);
        var valRight = GetReflectionValue<T>(field, right);
        var prevValRight = GetReflectionValue<T>(field, prevRight);

        if (valLeft != null || valRight != null)
        {
            bool dLeft = valLeft != null && !valLeft.Equals(prevValLeft);
            bool dRight = valRight != null && !valRight.Equals(prevValRight);
            if (!dLeft && !dRight)
                return null;

            var sb = new StringBuilder();
            if (dLeft)
                sb.Append(padString($"{field.Name}: {formatter?.Invoke(valLeft) ?? valLeft.ToString()}", hOffset));
            else if (dRight)
                sb.Append(padString(string.Empty, hOffset));

            if (dRight)
                sb.Append($"{field.Name}: {formatter?.Invoke(valRight) ?? valRight.ToString()}");

            sb.AppendLine();
            return sb.ToString();
        }

        return null;
    }

    private static double frameDelta = 0;
    private static void AnalyzeOne(Tuple<double, SerializableHapticState?, SerializableHapticState?> state,
        Tuple<double, SerializableHapticState?, SerializableHapticState?>? lastState, bool printLive, StreamWriter? fs)
    {
        bool saveAnalysis = fs != null;

        void writeLog(string message)
        {
            if (printLive)
                Console.Write(message);
            if (saveAnalysis)
                fs?.Write(message);
        }

        void writeLogLine(string message = "")
        {
            if (printLive)
                Console.WriteLine(message);
            if (saveAnalysis)
                fs?.WriteLine(message);
        }

        void writeReal(string message, bool rewriteLine = false)
        {
            if (!printLive)
            {
                if (rewriteLine)
                {
                    message += new string(' ', 100 - message.Length);
                    Console.CursorLeft = 0;
                }

                Console.Write(message);
            }
        }

        void writeRealLine(string message)
        {
            if (!printLive)
                Console.WriteLine(message);
        }

        SerializableHapticState? prevLeft = lastState?.Item2;
        SerializableHapticState? prevRight = lastState?.Item3;

        bool hit = false;
        var (_, left, right) = state;
        foreach (var field in typeof(SerializableHapticState).GetFields(BindingFlags.Public | BindingFlags.Instance))
        {
            if (field.FieldType == typeof(float))
            {
                var msg = ComputeLogValue<float>(field, left, prevLeft, right, prevRight);
                if(msg != null)
                {
                    hit = true;
                    writeLog(msg);
                }
            }
            else if (field.FieldType == typeof(uint))
            {
                var msg = ComputeLogValue<uint>(field, left, prevLeft, right, prevRight,
                    (v) =>
                    {
                        if (v > 127)
                            return $"{v} :: {v:X8}";
                        return v.ToString();
                    });
                if(msg != null)
                {
                    hit = true;
                    writeLog(msg);
                }
            }
            else if (field.FieldType == typeof(byte))
            {
                var msg = ComputeLogValue<byte>(field, left, prevLeft, right, prevRight);
                if(msg != null)
                {
                    hit = true;
                    writeLog(msg);
                }
            }
        }

        if (hit)
        {
            if(lastState != null)
            {
                writeLogLine($"DT: {(state.Item1 - lastState.Item1) + frameDelta:N2}ms");
                frameDelta = 0;
            }
        }
        else
        {
            if(lastState != null)
                frameDelta += state.Item1 - lastState.Item1;
        }
    }


    private static void Analyze(List<Tuple<double, SerializableHapticState?, SerializableHapticState?>> steps, bool saveAnalysis, bool printLive, string? overridePath = null)
    {
        string? path = null;
        StreamWriter? fs = null;
        if(saveAnalysis)
        {
            path = overridePath ?? $"Analysis-{DateTime.Now:yy-MM-dd_hh-mm}.txt";
            fs = new StreamWriter(path, printLive);
        }

        void writeLog(string message)
        {
            if(printLive)
                Console.Write(message);
            if(saveAnalysis)
                fs?.Write(message);
        }

        void writeLogLine(string message = "")
        {
            if(printLive)
                Console.WriteLine(message);
            if(saveAnalysis)
                fs?.WriteLine(message);
        }

        void writeReal(string message, bool rewriteLine = false)
        {
            if(!printLive)
            {
                if (rewriteLine)
                {
                    message += new string(' ', 100 - message.Length);
                    Console.CursorLeft = 0;
                }

                Console.Write(message);
            }
        }
        void writeRealLine(string message)
        {
            if(!printLive)
                Console.WriteLine(message);
        }

        //middleVibrationAmplitude: 3212836864 :: BF800000
        int hOffset = 50;

        writeLog(padString("Left:", hOffset));
        writeLogLine("Right:");
        writeLogLine(new string('=', hOffset*2));
        
        SerializableHapticState? prevLeft = null;
        SerializableHapticState? prevRight = null;
        double pStep = 0;

        writeReal("0%");
        for (var i = 0; i < steps.Count; i++)
        {
            var step = steps[i];
            var prevStep = (i > 0 ? steps[i - 1] : null);
            var progress = (float)i / steps.Count;
            writeReal($"{(int)MathF.Ceiling(progress * 100)}%", true);
            AnalyzeOne(step, prevStep, printLive, fs);
            pStep = step.Item1;
        }

        writeRealLine("");
        writeRealLine($"Analysis saved to {path}");
        fs?.Close();
        fs?.Dispose();
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
        Tuple<double, SerializableHapticState?, SerializableHapticState?>? lastState = null;
        StreamWriter? fs = null;
        if (record)
            fs = new StreamWriter(path.Replace("HapticCapture", "Analysis"), true);
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
                var state = new Tuple<double, SerializableHapticState?, SerializableHapticState?>(time.TotalMilliseconds, left, right);
                AnalyzeOne(state, lastState, liveAnalyze, fs);
                lastState = state;
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