using System;
using System.Collections.Generic;

namespace PKG
{
    /// <summary>
    /// Base
    /// </summary>
    public partial class Base : ISerde
    {
        /// <summary>
        /// S1
        /// </summary>
        public int S1 { get; set; }

        /// <summary>
        /// S2
        /// </summary>
        public string S2 { get; set; }

        public ushort GetTypeid() => 11;

        public int Read(xx.ObjManager om, xx.DataReader data)
        {
            int err;
            if ((err = data.ReadFiexd(out uint siz)) != 0) return err;
            int endoffset = (int)(data.Offset - sizeof(uint) + siz);

            if (data.Offset >= endoffset)
                this.S1 = default;
            else if ((err = om.ReadFrom(data, out int __s1)) == 0)
                this.S1 = __s1;
            else return err;

            if (data.Offset >= endoffset)
                this.S2 = default;
            else if ((err = om.ReadFrom(data, out string __s2)) == 0)
                this.S2 = __s2;
            else return err;

            if (data.Offset > endoffset)
                throw new IndexOutOfRangeException($"typeid:{ GetTypeid()} class: Foo offset error");
            else
                data.Offset = endoffset;
            return 0;
        }

        public void Write(xx.ObjManager om, xx.Data data)
        {
            var bak = data.Length;
            data.WriteFiexd(sizeof(uint));
            om.WriteTo(data, this.S1);
            om.WriteTo(data, this.S2);
            data.WriteFiexdAt(bak, (uint)(data.Length - bak));
        }
    }
    /// <summary>
    /// Foo
    /// </summary>
    public partial class Foo : PKG.Base,ISerde
    {
        public int P1 { get; set; }

        public float P2 { get; set; }

        public string P3 { get; set; }

        public byte[] Buff { get; set; }

        public List<uint> Data { get; set; }

        public PKG.P.Point Position { get; set; }

        public PKG.P.Point Position2 { get; set; }

        public PKG.Foo My { get; set; }

        public List<PKG.P.Point> Positions { get; set; }

        public new ushort GetTypeid() => 13;

        public new int Read(xx.ObjManager om, xx.DataReader data)
        {
            base.Read(om, data);

            int err;
            if ((err = data.ReadFiexd(out uint siz)) != 0) return err;
            int endoffset = (int)(data.Offset - sizeof(uint) + siz);

            if (data.Offset >= endoffset)
                this.P1 = default;
            else if ((err = om.ReadFrom(data, out int __p1)) == 0)
                this.P1 = __p1;
            else return err;

            if (data.Offset >= endoffset)
                this.P2 = default;
            else if ((err = om.ReadFrom(data, out float __p2)) == 0)
                this.P2 = __p2;
            else return err;

            if (data.Offset >= endoffset)
                this.P3 = default;
            else if ((err = om.ReadFrom(data, out string __p3)) == 0)
                this.P3 = __p3;
            else return err;

            if (data.Offset >= endoffset)
                this.Buff = default;
            else if ((err = om.ReadFrom(data, out byte[] __buff)) == 0)
                this.Buff = __buff;
            else return err;

            if (data.Offset >= endoffset)
                this.Data = default;
            else if ((err = om.ReadFrom(data, out List<uint> __data)) == 0)
                this.Data = __data;
            else return err;

            if (data.Offset >= endoffset)
                this.Position = default;
            else if ((err = om.ReadObj(data, out PKG.P.Point __position)) == 0)
                this.Position = __position;
            else return err;

            if (data.Offset >= endoffset)
                this.Position2 = default;
            else if ((err = om.ReadObj(data, out PKG.P.Point __position2)) == 0)
                this.Position2 = __position2;
            else return err;

            if (data.Offset >= endoffset)
                this.My = default;
            else if ((err = om.ReadObj(data, out PKG.Foo __my)) == 0)
                this.My = __my;
            else return err;

            if (data.Offset >= endoffset)
                this.Positions = default;
            else if ((err = om.ReadObj(data, out List<PKG.P.Point> __positions)) == 0)
                this.Positions = __positions;
            else return err;

            if (data.Offset > endoffset)
                throw new IndexOutOfRangeException($"typeid:{ GetTypeid()} class: Foo offset error");
            else
                data.Offset = endoffset;
            return 0;
        }

        public new void Write(xx.ObjManager om, xx.Data data)
        {
            base.Write(om, data);
            var bak = data.Length;
            data.WriteFiexd(sizeof(uint));
            om.WriteTo(data, this.P1);
            om.WriteTo(data, this.P2);
            om.WriteTo(data, this.P3);
            om.WriteTo(data, this.Buff);
            om.WriteTo(data, this.Data);
            om.WriteObj(data, this.Position);
            om.WriteObj(data, this.Position2);
            om.WriteObj(data, this.My);
            om.WriteObj(data, this.Positions);
            data.WriteFiexdAt(bak, (uint)(data.Length - bak));
        }
    }
}

namespace PKG.P
{
    /// <summary>
    /// Ponit
    /// </summary>
    public partial class Point : ISerde
    {
        public int X { get; set; }

        public int Y { get; set; }

        public double? Z { get; set; }

        public ushort GetTypeid() => 12;

        public int Read(xx.ObjManager om, xx.DataReader data)
        {
            int err;
            if ((err = om.ReadFrom(data, out int __x)) == 0)
            this.X = __x;
            else return err;

            if ((err = om.ReadFrom(data, out int __y)) == 0)
            this.Y = __y;
            else return err;

            if ((err = om.ReadFrom(data, out double? __z)) == 0)
            this.Z = __z;
            else return err;

            return 0;
    }

        public void Write(xx.ObjManager om, xx.Data data)
        {
            om.WriteTo(data, this.X);
            om.WriteTo(data, this.Y);
            om.WriteTo(data, this.Z);
        }
    }
}


public static class CodeGen_master
{
    public const string md5 = "#*MD5<a8eeae054983d066ef32b5b6b11a82a3>*#"; 
    public static void Register()
    {
         xx.ObjManager.Register<PKG.Base>(11);
         xx.ObjManager.Register<PKG.Foo>(13);
         xx.ObjManager.Register<PKG.P.Point>(12);
    }
}
