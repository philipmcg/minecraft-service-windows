using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using LibNbt;
using Utilities;

namespace WorldSwitch
{
    class Program
    {
        static void Usage()
        {
            Console.Error.WriteLine("usage: WorldSwitch.exe <inifile> <command> params...");
            Console.ReadKey();
        }

        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Usage();
                return;
            }

            var controller = new Controller(args);
            bool success = controller.PerformCommand();
            if (!success)
                Usage();
        }
    }

    class Controller
    {
        string[] args;
        string inifile;
        string command;
        VariableBin var;

        public Controller(string[] args)
        {
            this.args = args;
            this.inifile = args[0];
            this.command = args[1];

            var = new VariableBin();
            var.LoadFromFile(inifile);
        }

        public bool PerformCommand()
        {
            if (command == "worldswitch")
                return Try(WorldSwitch);
            else if (command == "teleport")
                return Try(Teleport);
            else if (command == "get_coords")
                return Try(GetCoords);

            return false;
        }

        void Teleport()
        {
            string player = args[2];
            string world = args[3];
            string coords = args[4];
            var operation = new Teleporter(var);
            operation.Teleport(player, world, coords);
        }

        void GetCoords()
        {
            string player = args[2];
            string world = args[3];
            var operation = new CoordinateGetter(var);
            operation.GetCoords(player, world);
        }

        void WorldSwitch()
        {
            string player = args[2];
            string world1 = args[3];
            string world2 = args[4];
            var operation = new WorldSwitcher(var);
            operation.Switch(world1, world2, player);
        }

        bool Try(Action action)
        {
            try
            {
                action();
                Console.WriteLine("success,");
                return true;
            }
            catch(Exception e)
            {
                File.WriteAllText("WorldSwitch.exe.log", e.ToString());
                return false;
            }
        }
    }

    class NbtHolder : IDisposable
    {
        public readonly NbtFile File;
        string path;
        public NbtHolder(string filename)
        {
            this.path = filename;
            File = new NbtFile(filename);
            File.LoadFile();
        }

        public void Dispose()
        {
            File.SaveFile(path);
        }
    }

    class Operation
    {
        protected VariableBin var;
        public string ErrorMessage = "Error";

        public Operation(VariableBin var)
        {
            this.var = var;
        }

        protected string PlayerDirectory(string worldname)
        {
            return Path.Combine(var.Str[worldname], "players");
        }

        protected NbtHolder LoadPlayer(string world, string player)
        {
            var playerDir = PlayerDirectory(world);
            if (!Directory.Exists(playerDir))
                ErrorMessage = string.Format("player folder not found for world: {0}", world);
            string playerFile = Path.Combine(playerDir, player + ".dat");
            if (!File.Exists(playerFile))
                ErrorMessage = string.Format("player file not found for world: {0} and player: {1}", world, player);

            return LoadNbt(playerFile);
        }

        protected NbtHolder LoadNbt(string path)
        {
            return new NbtHolder(path);
        }

        protected LibNbt.Tags.NbtList GetPos(NbtHolder player)
        {
            var pos = player.File.RootTag.Tags.First(t => t.Name == "Pos") as LibNbt.Tags.NbtList;
            return pos;
        }
    }
    
    struct Coordinates 
    {
        const char delimiter = ':';
        public double x;
        public double y;
        public double z;

        public Coordinates(string str) 
        {
            var list = str.Split(delimiter);
            x = double.Parse(list[0]);
            y = double.Parse(list[1]);
            z = double.Parse(list[2]);
        }

        public Coordinates(LibNbt.Tags.NbtList pos)
        {
            x = (pos[0] as LibNbt.Tags.NbtDouble).Value;
            y = (pos[1] as LibNbt.Tags.NbtDouble).Value;
            z = (pos[2] as LibNbt.Tags.NbtDouble).Value;
        }

        public void ApplyCoordinatesToPos(LibNbt.Tags.NbtList pos)
        {
            (pos[0] as LibNbt.Tags.NbtDouble).Value = this.x;
            (pos[1] as LibNbt.Tags.NbtDouble).Value = this.y;
            (pos[2] as LibNbt.Tags.NbtDouble).Value = this.z;
        }

        public override string ToString()
        {
            return "" + x + delimiter + y + delimiter + z;
        }
    }

    class CoordinateGetter : Operation
    {
        public CoordinateGetter(VariableBin var) : base(var) { }

        public void GetCoords(string player, string world)
        {
            using (var playerTag = LoadPlayer(world, player))
            {
                var pos = GetPos(playerTag);
                var coords = new Coordinates(pos);
                Console.WriteLine(coords.ToString());
            }
        }
    }

    class Teleporter : Operation
    {
        public Teleporter(VariableBin var) : base(var) { }

        public void Teleport(string player, string world, string coordinates)
        {
            var coords = new Coordinates(coordinates);
            using (var playerTag = LoadPlayer(world, player))
            {
                var pos = GetPos(playerTag);
                coords.ApplyCoordinatesToPos(pos);
            }
        }
    }

    class WorldSwitcher : Operation
    {
        public WorldSwitcher(VariableBin var) : base(var) { }

        public void Switch(string world1, string world2, string player)
        {
            var player1 = LoadPlayer(world1, player);
            var player2 = LoadPlayer(world2, player);

            SwapInventories(player1.File, player2.File);

            player1.Dispose();
            player2.Dispose();
        }

        void SwapInventories(NbtFile player1, NbtFile player2)
        {
            var inventory1 = player1.RootTag.Tags.First(t => t.Name == "Inventory") as LibNbt.Tags.NbtList;
            var inventory2 = player2.RootTag.Tags.First(t => t.Name == "Inventory") as LibNbt.Tags.NbtList;

            player1.RootTag.Tags.Remove(inventory1);
            player2.RootTag.Tags.Remove(inventory2);

            player1.RootTag.Tags.Add(inventory2);
            player2.RootTag.Tags.Add(inventory1);
        }
    }
}
