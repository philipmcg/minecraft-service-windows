using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using LibNbt;
using Utilities;

namespace WorldSwitch
{
    // the parameters to this program are the names of the two worlds to switch between, and the name of the player to switch.
    // the program's ini file contains a path to each world's folder.
    // For instance, if the world to switch from comes in as "world1", then there must be an entry in the ini file like $world1_path=C:\Minecraft\Server1\
    // and inside this Server1 folder must be the "world" folder.
    // So for instance:  WorldSwitch.exe world1 world2 PhilipM
    // will swap PhilipM's inventory between the two worlds.
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 4)
            {
                Console.WriteLine("usage: WorldSwitch.exe <inifile> <world1> <world2> <player>");
                Console.ReadKey();
                return;
            }

            string inifile = args[0];
            string world1 = args[1];
            string world2 = args[2];
            string player = args[3];

            WorldSwitcher switcher = new WorldSwitcher();
            try
            {
                switcher.Switch(inifile, world1, world2, player);
            }
            catch(Exception e)
            {
                Console.WriteLine(switcher.ErrorMessage);
                File.WriteAllText("WorldSwitch.exe.log", e.ToString());
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

    class WorldSwitcher {

        VariableBin var;
        public string ErrorMessage = "Error";

        public WorldSwitcher()
        {
        }

        public void Switch(string inifile, string world1, string world2, string player)
        {
            var = new VariableBin();
            var.LoadFromFile(inifile);

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

        string PlayerDirectory(string worldname)
        {
            return Path.Combine(var.Str[worldname],"players");
        }

        NbtHolder LoadPlayer(string world, string player)
        {
            var playerDir = PlayerDirectory(world);
            if (!Directory.Exists(playerDir))
                ErrorMessage = string.Format("player folder not found for world: {0}", world);
            string playerFile = Path.Combine(playerDir, player + ".dat");
            if (!File.Exists(playerFile))
                ErrorMessage = string.Format("player file not found for world: {0} and player: {1}", world, player);

            return LoadNbt(playerFile);
        }

        NbtHolder LoadNbt(string path)
        {
            return new NbtHolder(path);
        }
    }
}
