/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 Copyright (c) 2021 Bytedance Inc.

 https://adxe.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "AppDelegate.h"
#include "HelloWorldScene.h"

#define USE_AUDIO_ENGINE 1

#if USE_AUDIO_ENGINE
#include "audio/include/AudioEngine.h"
#endif

//USING_NS_CC;

static cocos2d::Size designResolutionSize = cocos2d::Size(720, 1280);
static cocos2d::Size smallResolutionSize = cocos2d::Size(320, 480);
static cocos2d::Size mediumResolutionSize = cocos2d::Size(768, 1024);
static cocos2d::Size largeResolutionSize = cocos2d::Size(1536, 2048);

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate() 
{
#if USE_AUDIO_ENGINE
    cocos2d::AudioEngine::end();
#endif
}

// if you want a different context, modify the value of glContextAttrs
// it will affect all platforms
void AppDelegate::initGLContextAttrs()
{
    // set OpenGL context attributes: red,green,blue,alpha,depth,stencil,multisamplesCount
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8, 0};

    cocos2d::GLView::setGLContextAttrs(glContextAttrs);
}

// if you want to use the package manager to install more packages,  
// don't modify or remove this function
static int register_all_packages()
{
    return 0; //flag for packages manager
}


/*****************************************************************************************************************************************/
// 直接在此附加 lua 功能以示说明

#include <xx_lua_bind.h>

// 单例区
inline static AppDelegate* _appDelegate = nullptr;
inline static cocos2d::Director* _director = nullptr;
inline static lua_State* _luaState = nullptr;
inline static xx::Lua::Func _globalUpdate;  // 注意：restart 功能实现时需先 Reset

// 模板适配区
namespace xx::Lua {
    // 基类 指针方式 meta
    template<>
    struct MetaFuncs<cocos2d::Node*, void> {
        using U = cocos2d::Node*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            SetType<U>(L);
            SetFieldCClosure(L, "setPosition", [](auto L)->int { 
                To<U>(L)->setPosition(To<float>(L, 2), To<float>(L, 3));
                return 0;
            });
            SetFieldCClosure(L, "getPosition", [](auto L)->int { 
                auto&& pos = To<U>(L)->getPosition();
                return Push(L, pos.x, pos.y);
            });
            SetFieldCClosure(L, "addChild", [](auto L)->int {
                To<U>(L)->addChild(To<cocos2d::Node*>(L, 2));
                return 0;
            });
        }
    };
    // 派生类 指针方式 meta
    template<>
    struct MetaFuncs<cocos2d::Scene*, void> {
        using U = cocos2d::Scene*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
        }
    };
    template<>
    struct MetaFuncs<cocos2d::Sprite*, void> {
        using U = cocos2d::Sprite*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "setTexture", [](auto L)->int {
                To<U>(L)->setTexture(To<char const*>(L, 2));
                return 0;
                });
        }
    };
    // 指针方式 push + to
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>> && std::is_base_of_v<cocos2d::Node, std::remove_pointer_t<std::decay_t<T>>>>> {
        using U = std::decay_t<T>;
        static int Push(lua_State* const& L, T&& in) {
            return PushUserdata<U>(L, in);
        }
        static void To(lua_State* const& L, int const& idx, T& out) {
            EnsureType<U>(L, idx);  // 只在 Debug 生效用 AssertType
            out = *(U*)lua_touserdata(L, idx);
        }
    };
}

void luaBinds(AppDelegate* ad) {
    namespace XL = xx::Lua;

    // 创建环境
    _appDelegate = ad;
    _director = cocos2d::Director::getInstance();
    auto L = _luaState = luaL_newstate();
    luaL_openlibs(_luaState);

    // 简单给 lua 塞几个 cocos 函数
    XL::SetGlobalCClosure(L, "cc_scene_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Scene::create());
    });

    XL::SetGlobalCClosure(L, "cc_director_runWithScene", [](auto L) -> int {
        auto&& scene = XL::To<cocos2d::Scene*>(L, 1);
        _director->runWithScene(scene);
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_sprite_create", [](auto L) -> int {
        auto r = lua_gettop(L)
            ? cocos2d::Sprite::create(XL::To<char const*>(L, 1))
            : cocos2d::Sprite::create();
        return XL::Push(L, r);
    });

    // 注册 lua 函数回调到 _globalUpdate
    XL::SetGlobalCClosure(L, "cc_register_global_update", [](auto L) -> int {
        To(L, 1, _globalUpdate);
        return 0;
    });

    // 产生 lua 帧回调
    _director->getScheduler()->schedule([](float delta) {
        if (_globalUpdate) {
            auto r = XL::Try(_luaState, [&] {
                _globalUpdate.Call(delta);
            });
            if (r) {
                xx::CoutN(r.m);
            }
        }
    }, (void*)ad, 1.0f / 60, false, "AppDelegate");

    XL::AssertTop(L, 0);

    // 模拟加载入口 lua 文件源码
    auto luaSrc = R"(
local scene = cc_scene_create()
cc_director_runWithScene(scene)

local spr = cc_sprite_create()
spr:setTexture("HelloWorld.png")
spr:setPosition(300, 300)
scene:addChild(spr)

cc_register_global_update(function(delta)
    local x,y = spr:getPosition()
    spr:setPosition( x + delta * 20, y )
end)
)";

    auto r = XL::Try(L, [&] {
        xx::Lua::DoString(L, luaSrc);
    });
    if (r) {
        xx::CoutN(r.m);
    }
}

/*****************************************************************************************************************************************/


bool AppDelegate::applicationDidFinishLaunching() {
    // initialize director
    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if(!glview) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
        glview = cocos2d::GLViewImpl::createWithRect("adxe_lua", cocos2d::Rect(0, 0, designResolutionSize.width, designResolutionSize.height));
#else
        glview = cocos2d::GLViewImpl::create("adxe_lua");
#endif
        director->setOpenGLView(glview);
    }

    // turn on display FPS
    director->setDisplayStats(true);

    // set FPS. the default value is 1.0/60 if you don't call this
    director->setAnimationInterval(1.0f / 60);

    // Set the design resolution
    glview->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height, ResolutionPolicy::NO_BORDER);
    auto frameSize = glview->getFrameSize();
    // if the frame's height is larger than the height of medium size.
    if (frameSize.height > mediumResolutionSize.height)
    {        
        director->setContentScaleFactor(MIN(largeResolutionSize.height/designResolutionSize.height, largeResolutionSize.width/designResolutionSize.width));
    }
    // if the frame's height is larger than the height of small size.
    else if (frameSize.height > smallResolutionSize.height)
    {        
        director->setContentScaleFactor(MIN(mediumResolutionSize.height/designResolutionSize.height, mediumResolutionSize.width/designResolutionSize.width));
    }
    // if the frame's height is smaller than the height of medium size.
    else
    {        
        director->setContentScaleFactor(MIN(smallResolutionSize.height/designResolutionSize.height, smallResolutionSize.width/designResolutionSize.width));
    }

    register_all_packages();

    //// create a scene. it's an autorelease object
    //auto scene = utils::createInstance<HelloWorld>();

    //// run
    //director->runWithScene(scene);

    luaBinds(this);

    return true;
}

// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground() {
    cocos2d::Director::getInstance()->stopAnimation();

#if USE_AUDIO_ENGINE
    cocos2d::AudioEngine::pauseAll();
#endif
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground() {
    cocos2d::Director::getInstance()->startAnimation();

#if USE_AUDIO_ENGINE
    cocos2d::AudioEngine::resumeAll();
#endif
}
