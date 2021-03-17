#pragma warning disable CS0169
using TemplateLibrary;



namespace PKG.P
{
    [TypeId(12), Desc("Ponit")]
    class Point
    {
        int X;
        int Y;
        double? Z;
    }
}


namespace PKG
{

    [TypeId(11), Desc("Base"),Compatible]
    class Base
    {
        [Desc("S1")]
        int S1;
        [Desc("S2")]
        string S2;
    }


    [TypeId(13), Desc("Foo"), Compatible]
    class Foo : Base
    {
        int P1;
        float P2;
        string P3;
        byte[] Buff;
        List<uint> Data;
        Shared<PKG.P.Point> Position;
        Shared<PKG.P.Point> Position2;
        Shared<Foo> My;
        List<Shared<PKG.P.Point>> Positions;
    }

}