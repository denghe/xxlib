using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class MainScript : MonoBehaviour
{
    public static MainScript instance = null;
    public MainScript()
    {
        instance = this;
    }

    public Camera cam; // bind in editor
    public GameObject myPrefab; // bind in editor

    private xx.Dict<int, GameObject> gs = new xx.Dict<int, GameObject>();
    private int autoId = 0; // gs[++autoId] = new GameObject
    private IntPtr logic;


    public int SpriteNew()
    {
        return gs.Add(
            ++autoId
            , Instantiate(myPrefab, new Vector3(0, 0, 0), Quaternion.identity)
            ).index;
    }

    public void DeleteSprite(int selfIndex)
    {
        Destroy(gs.ValueAt(selfIndex));
        gs.RemoveAt(selfIndex);
    }

    public void SpritePos(int selfIndex, float x, float y)
    {
        gs.ValueAt(selfIndex).transform.position = new Vector3(x, y, 0);
    }


    void Start()
    {
        Application.targetFrameRate = 60;
        DllFuncs.SetFuncs();
        logic = DllFuncs.LogicNew();
    }

    private void OnGUI()
    {
        switch (Event.current.type)
        {
            case EventType.TouchDown:
            case EventType.MouseDown:
                {
                    var p = cam.ScreenToWorldPoint(Event.current.mousePosition);
                    //DllFuncs.LogicTouchDown(logic, 0, p.x, 1080 - p.y);
                    DllFuncs.LogicTouchDown(logic, 0, 100, 100);
                    break;
                }
            case EventType.MouseUp:
            case EventType.TouchUp:
                {
                    var p = cam.ScreenToWorldPoint(Event.current.mousePosition);
                    DllFuncs.LogicTouchUp(logic, 0, p.x, 1080 - p.y);
                    break;
                }
            case EventType.TouchMove:
            case EventType.MouseMove:
            case EventType.MouseDrag:
                {
                    var p = cam.ScreenToWorldPoint(Event.current.mousePosition);
                    DllFuncs.LogicTouchMove(logic, 0, p.x, 1080 - p.y);
                    break;
                }
        }
        if (Input.GetKeyDown(KeyCode.Space))
        {
            DllFuncs.LogicTouchDown(logic, 0, 100, 100);
        }
    }

    void Update()
    {
        DllFuncs.LogicUpdate(logic, 0);
    }

    void OnDestroy()
    {
        Debug.Log("OnDestroy");
        if (logic != IntPtr.Zero)
        {
            DllFuncs.LogicDelete(logic);
            logic = IntPtr.Zero;
        }
    }
}

public static class DllFuncs
{
#if !UNITY_EDITOR && UNITY_IPHONE
    const string DLL_NAME = "__Internal";
#else
    const string DLL_NAME = "vs_android_so_test1";
#endif

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int FI();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void FVI(int i1);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void FVIFF(int i1, float f1, float f2);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern int SetFunc_SpriteNew(FI f);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern int SetFunc_SpriteDelete(FVI f);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern int SetFunc_SpritePos(FVIFF f);

    [AOT.MonoPInvokeCallback(typeof(FI))]
    public static int Func_SpriteNew()
    {
        return MainScript.instance.SpriteNew();
    }
    [AOT.MonoPInvokeCallback(typeof(FVI))]
    public static void Func_SpriteDelete(int selfIndex)
    {
        MainScript.instance.DeleteSprite(selfIndex);
    }
    [AOT.MonoPInvokeCallback(typeof(FVIFF))]
    public static void Func_SpritePos(int selfIndex, float x, float y)
    {
        MainScript.instance.SpritePos(selfIndex, x, y);
    }
    public static void SetFuncs()
    {
        SetFunc_SpriteNew(Func_SpriteNew);
        SetFunc_SpriteDelete(Func_SpriteDelete);
        SetFunc_SpritePos(Func_SpritePos);
    }


    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern IntPtr LogicNew();
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicUpdate(IntPtr self, float delta);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicDelete(IntPtr self);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicTouchDown(IntPtr self, int idx, float x, float y);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicTouchUp(IntPtr self, int idx, float x, float y);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicTouchCancel(IntPtr self, int idx, float x, float y);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicTouchMove(IntPtr self, int idx, float x, float y);
}
