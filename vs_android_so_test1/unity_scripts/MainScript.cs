using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class MainScript : MonoBehaviour
{
    public GameObject myPrefab;

    private xx.Dict<int, GameObject> gos = new xx.Dict<int, GameObject>();
    private int autoId = 0;
    private IntPtr logic;

    private static MainScript instance = null;

    // Start is called before the first frame update
    void Start()
    {
        instance = this;
        DllFuncs.SetFunc_SpriteNew(() => {
            var r = instance.gos.Add(++autoId, Instantiate(myPrefab, new Vector3(0, 0, 0), Quaternion.identity));
            //Debug.Log($"new {r.index}");
            return r.index;
        });

        DllFuncs.SetFunc_DeleteSprite((int self_) => {
            var go = instance.gos.ValueAt(self_);
            //Debug.Log($"destroy {go}");
            GameObject.Destroy(go);
            instance.gos.RemoveAt(self_);
        });

        DllFuncs.SetFunc_SpritePos((int self_, float x, float y) => {
            var go = instance.gos.ValueAt(self_);
            //Debug.Log($"pos{x}, {y} go = {go}");
            instance.gos.ValueAt(self_).transform.position = new Vector3(x, y, 0); 
        });

        logic = DllFuncs.LogicNew();
        Debug.Log(logic.ToString());
    }

    // Update is called once per frame
    void Update()
    {
        DllFuncs.LogicUpdate(logic, 0);
    }

    // todo: DllFuncs.LogicDelete( logic )
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
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern int SetFunc_DeleteSprite(FVI f);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern int SetFunc_SpritePos(FVIFF f);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern IntPtr LogicNew();
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicUpdate(IntPtr self, float delta);
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)] public static extern void LogicDelete(IntPtr self);
}
