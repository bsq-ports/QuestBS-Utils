#include "utils.hpp"
#include "utils-logging.hpp"
#include "beatsaber-hook/utils/utils.h"
#include "beatsaber-hook/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/utils/il2cpp-utils.hpp"


bool CheckAssetClass(Il2CppObject* asset) {
    auto* cAsset = CRASH_UNLESS(il2cpp_functions::object_get_class(asset));
    if (cAsset == il2cpp_functions::defaults->string_class) {
        Il2CppString* str = reinterpret_cast<Il2CppString*>(asset);
        getLogger().error("asset is string?! '%s'", to_utf8(csstrtostr(str)).c_str());
        return false;
    } else {
        getLogger().info("asset is %s ?", il2cpp_utils::ClassStandardName(cAsset).c_str());
    }
    return true;
}

// Note: if too much time passes since AssetComplete, asset becomes corrupted?
Il2CppObject* bs_utils::AssetImporter::InstantiateAsset(std::string_view nameSpace, std::string_view klass, std::string_view method) const {
    RET_0_UNLESS(LoadedAsset());
    // RET_0_UNLESS(CheckAssetClass(asset));

    auto* ret = RET_0_UNLESS(il2cpp_utils::RunMethod(nameSpace, klass, method, const_cast<Il2CppObject*>(asset)));
    getLogger().info("Instantiated Asset Object (%p)", ret);
    return ret;
}

void bs_utils::AssetImporter::AssetComplete(AssetImporter* obj, Il2CppObject* asyncOp) {
    CRASH_UNLESS(il2cpp_utils::GetPropertyValue<bool>(asyncOp, "isDone").value_or(false));
    obj->asset = RET_V_UNLESS(il2cpp_utils::GetPropertyValue(asyncOp, "asset"));
    // RET_V_UNLESS(CheckAssetClass(asset));

    if (obj->whenDone) {
        obj->whenDone(obj);
    }
}

bool bs_utils::AssetImporter::LoadAsset(std::string_view assetNameS) {
    if (assetAsync && !LoadedAsset()) {
        return (assetNameS.empty() || !assetName || (assetNameS == to_utf8(csstrtostr(assetName))));
    }
    if (!assetNameS.empty()) {
        assetName = CRASH_UNLESS(il2cpp_utils::createcsstr(assetNameS));
        asset = nullptr;
    }

    RET_0_UNLESS(assetType);
    RET_0_UNLESS(assetName);
    getLogger().info("Loading asset '%s'", to_utf8(csstrtostr(assetName)).c_str());

    permissionToLoadAsset = true;
    if (!LoadedAssetBundle())
        return false;
    if (LoadedAsset())
        return true;
    asset = nullptr;
    assetAsync = RET_0_UNLESS(il2cpp_utils::RunMethod(assetBundle, "LoadAssetAsync", assetName, assetType));

    auto method = il2cpp_utils::FindMethodUnsafe(assetAsync, "add_completed", 1);
    auto action = il2cpp_utils::MakeAction(method, 0, this, AssetComplete);

    RET_0_UNLESS(il2cpp_utils::RunMethod(assetAsync, method, action));
    getLogger().info("Began loading asset async");
    return true;
}

void bs_utils::AssetImporter::AssetBundleComplete(AssetImporter* obj, Il2CppObject* asyncOp) {
    obj->assetBundle = RET_V_UNLESS(il2cpp_utils::GetPropertyValue(asyncOp, "assetBundle"));
    if (obj->permissionToLoadAsset)
        obj->LoadAsset();
}

bool bs_utils::AssetImporter::LoadAssetBundle(bool alsoLoadAsset) {
    permissionToLoadAsset = alsoLoadAsset;
    if (!pathExists) {
        pathExists = RET_0_UNLESS(il2cpp_utils::RunMethod<bool>("System.IO", "File", "Exists", assetFilePath));
    }
    RET_0_UNLESS(pathExists);
    if (!bundleAsync) {
        bundleAsync = RET_0_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "AssetBundle", "LoadFromFileAsync", assetFilePath));
        // TODO: change to SetPropertyValue(bundleAsync, "allowSceneActivation", true) when true becomes a valid param
        RET_0_UNLESS(il2cpp_utils::RunMethod(bundleAsync, "set_allowSceneActivation", true));

        auto method = RET_0_UNLESS(il2cpp_utils::FindMethodUnsafe(bundleAsync, "add_completed", 1));
        auto action = RET_0_UNLESS(il2cpp_utils::MakeAction(method, 0, this, AssetBundleComplete));

        RET_0_UNLESS(il2cpp_utils::RunMethod(bundleAsync, method, action));
        getLogger().info("Began loading bundle async");
    }
    return true;
}