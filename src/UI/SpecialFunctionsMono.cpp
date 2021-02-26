#include "SpecialFunctionsMono.h"

#include "OFS_Util.h"
#include "OpenFunscripter.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/class.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/image.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/object.h>

#include "reproc++/run.hpp"
#include "reproc++/reproc.hpp"

#include <unordered_set>

#include "imgui_internal.h"

MonoDomain* CustomMono::Domain = nullptr;

MonoAssembly* UserAssembly = nullptr;
MonoClass* ScriptContextType = nullptr;
MonoClass* ScriptType = nullptr;
MonoClass* ScriptActionType = nullptr;

CustomMono::CustomMono()
{
    InitMono();
    ReloadScripts();
}

CustomMono::~CustomMono()
{
}


void CustomMono::InitMono() noexcept
{
    if (Domain == nullptr)
    {
        auto& MonoBasePath = OpenFunscripter::ptr->settings->data().monoSettings.MonoPath;
        std::stringstream ss;
        ss << MonoBasePath << "lib";
        auto libPath = ss.str();
        ss.str("");
        ss << MonoBasePath << "etc";
        auto etcPath = ss.str();

        if (Util::FileExists(etcPath + "\\mono\\config")) {
            mono_set_dirs(libPath.c_str(), etcPath.c_str());
            Domain = mono_jit_init(NULL);
            mono_domain_set(Domain, false);
            Domain = mono_domain_create_appdomain("OFS Mono", NULL);
            mono_domain_set(Domain, false);
        }

        if (!Domain) {
            LOG_ERROR("Failed to initialize mono domain.");
        }
        else
        {
            //LOGF_INFO("mono: %s", mono_get_runtime_build_info());
            //SystemAssembly = mono_domain_assembly_open(Domain, extensionAssembly.c_str());
            //if (!SystemAssembly) { 
            //    LOGF_ERROR("Failed to load: %s", extensionAssembly.c_str()); 
            //}
        }
    }
}


void CustomMono::GetExtensionTypeName(const char** Namespace, const char** TypeName) noexcept
{
    MonoImage* image = mono_assembly_get_image(UserAssembly);
    MonoClass* extensionBaseClass =  mono_class_from_name(image, "OFS", "Extension");
    

    const MonoTableInfo* typesTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
    int rows = mono_table_info_get_rows(typesTable);
    for (int i = 0; i < rows; i++) {
        uint32_t cols[MONO_TYPEDEF_SIZE];

        mono_metadata_decode_row(typesTable, i, cols, MONO_TYPEDEF_SIZE);
        
        const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
        const char* nspace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);

        MonoClass* foundClass = mono_class_from_name_case(image, nspace, name);
        if (foundClass != extensionBaseClass) {
            if (mono_class_is_subclass_of(foundClass, extensionBaseClass, false)) {
                *Namespace = nspace;
                *TypeName = name;
                break;
            }
        }
    }
}

static MonoObject* GetScriptContext()
{
    LOG_DEBUG("OFS.Extension::GetContext");

    MonoObject* ctx = mono_object_new(CustomMono::Domain, ScriptContextType);
    mono_runtime_object_init(ctx); // call ctor

    auto scriptList = mono_class_get_field_from_name(ScriptContextType, "Scripts");
    auto scriptListClass = mono_class_from_mono_type(mono_field_get_type(scriptList));
    auto desc = mono_method_desc_new("List<Script>:Add(Script)", false);
    auto addMethod = mono_method_desc_search_in_class(desc, scriptListClass);
    mono_method_desc_free(desc);

    auto scriptListObj = mono_field_get_value_object(CustomMono::Domain, scriptList, ctx);

    auto scriptNameField = mono_class_get_field_from_name(ScriptType, "Name");
    auto scriptActionsField = mono_class_get_field_from_name(ScriptType, "actions");
    auto scriptActionsClass = mono_class_from_mono_type(mono_field_get_type(scriptActionsField));
    desc = mono_method_desc_new("List<ScriptAction>:Add(ScriptAction)", false);
    auto addActionMethod = mono_method_desc_search_in_class(desc, scriptActionsClass);
    mono_method_desc_free(desc);

    auto app = OpenFunscripter::ptr;
    for (auto& script : app->LoadedFunscripts)
    {
        MonoObject* scriptObj = mono_object_new(CustomMono::Domain, ScriptType);
        mono_runtime_object_init(scriptObj);

        auto scriptNameMonoString = mono_string_new(CustomMono::Domain, script->metadata.title.c_str());
        mono_field_set_value(scriptObj, scriptNameField, scriptNameMonoString);
        auto scriptActionsListObj = mono_field_get_value_object(CustomMono::Domain, scriptActionsField, scriptObj);

        for (auto action : script->Actions())
        {
            MonoObject* actionObj = mono_object_new(CustomMono::Domain, ScriptActionType);
            mono_runtime_object_init(actionObj);

            auto at = mono_class_get_field_from_name(ScriptActionType, "at");
            auto pos = mono_class_get_field_from_name(ScriptActionType, "pos");
            auto selected = mono_class_get_field_from_name(ScriptActionType, "selected");
            
            mono_field_set_value(actionObj, at, (void*)&action.at);
            mono_field_set_value(actionObj, pos, (void*)&action.pos);
            bool selectedVal = script->IsSelected(action);
            mono_field_set_value(actionObj, selected, (void*)&selectedVal);

            void* params[1];
            params[0] = actionObj;
            mono_runtime_invoke(addActionMethod, scriptActionsListObj, params, NULL);
        }

        void* params[1];
        params[0] = scriptObj;
        mono_runtime_invoke(addMethod, scriptListObj, params, NULL);
    }

    
    mono_free_method(addMethod);
    mono_free_method(addActionMethod);

    return ctx;
}

static void UpdateFromContext(MonoObject* ctx)
{
    LOG_DEBUG("OFS.Extension::UpdateFromContext");

    auto scriptList = mono_class_get_field_from_name(ScriptContextType, "Scripts");
    auto scriptListClass = mono_class_from_mono_type(mono_field_get_type(scriptList));
    auto desc = mono_method_desc_new("List<Script>:ToArray()", false);
    auto toArrayScriptsMethod = mono_method_desc_search_in_class(desc, scriptListClass);
    mono_method_desc_free(desc);


    auto scriptListObj = mono_field_get_value_object(CustomMono::Domain, scriptList, ctx);

    auto scriptNameField = mono_class_get_field_from_name(ScriptType, "Name");
    auto scriptActionsField = mono_class_get_field_from_name(ScriptType, "actions");
    auto scriptActionsClass = mono_class_from_mono_type(mono_field_get_type(scriptActionsField));
    desc = mono_method_desc_new("List<ScriptAction>:ToArray()", false);
    auto toArrayActionsMethod = mono_method_desc_search_in_class(desc, scriptActionsClass);
    mono_method_desc_free(desc);

    auto app = OpenFunscripter::ptr;

    MonoArray* scripts = (MonoArray*)mono_runtime_invoke(toArrayScriptsMethod, scriptListObj, NULL, NULL);
    auto scriptCount = mono_array_length(scripts);
    for (int i = 0; i < scriptCount; i++) {
        auto script = mono_array_get(scripts, MonoObject*, i);

        MonoArray* scriptActions = (MonoArray*)mono_runtime_invoke(toArrayActionsMethod, mono_field_get_value_object(CustomMono::Domain, scriptActionsField, script), NULL, NULL);
        auto actionCount = mono_array_length(scriptActions);
        
        std::unordered_set<FunscriptAction, FunscriptActionHashfunction> actions;
        std::unordered_set<FunscriptAction, FunscriptActionHashfunction> selection;

        auto at = mono_class_get_field_from_name(ScriptActionType, "at");
        auto pos = mono_class_get_field_from_name(ScriptActionType, "pos");
        auto selected = mono_class_get_field_from_name(ScriptActionType, "selected");

        for (int idx = 0; idx < actionCount; idx++) {
            FunscriptAction newAction;
            bool isSelected = false;
            auto action = mono_array_get(scriptActions, MonoObject*, idx);
            mono_field_get_value(action, at, &newAction.at);
            mono_field_get_value(action, pos, &newAction.pos);
            mono_field_get_value(action, selected, &isSelected);

            actions.insert(newAction);
            if (isSelected) { selection.insert(newAction); }
        }

        std::vector<FunscriptAction> tmpBuffer;
        tmpBuffer.clear();
        tmpBuffer.reserve(actions.size());
        tmpBuffer.insert(tmpBuffer.end(), actions.begin(), actions.end());
        app->LoadedFunscripts[i]->SetActions(tmpBuffer);

        tmpBuffer.clear();
        tmpBuffer.insert(tmpBuffer.end(), selection.begin(), selection.end());
        app->LoadedFunscripts[i]->SetSelection(tmpBuffer, true);
    }

    mono_free_method(toArrayScriptsMethod);
    mono_free_method(toArrayActionsMethod);
}

void CustomMono::RunExtension(const std::string& assembly) noexcept
{
    if (Domain == nullptr) return;
    UserAssembly = mono_domain_assembly_open(Domain, assembly.c_str());
    MonoImage* image =  mono_assembly_get_image(UserAssembly);
    const char* TypeName = nullptr; const char* Namespace = nullptr;
    GetExtensionTypeName(&Namespace, &TypeName);

    mono_add_internal_call("OFS.Extension::GetContext", (const void*)GetScriptContext);
    mono_add_internal_call("OFS.Extension::UpdateFromContext", (const void*)UpdateFromContext);

    ScriptContextType = mono_class_from_name(image, "OFS", "Context");
    ScriptType = mono_class_from_name(image, "OFS", "Script");
    ScriptActionType = mono_class_from_name(image, "OFS", "ScriptAction");


    if (TypeName) {
        LOGF_INFO("Found extension type: %s", TypeName);
        MonoClass* classType = mono_class_from_name(image, Namespace, TypeName);
        MonoClass* parent = mono_class_get_parent(classType);
        LOGF_DEBUG("Parent: %s", mono_class_get_name(parent));

        void* ptr = NULL;
        MonoMethod* method = nullptr;
        while (method = mono_class_get_methods(parent, &ptr))
        {
            if (strcmp("RunExtension", mono_method_get_name(method)) == 0) {
                break;
            }
            else { method = nullptr; }
        }


        if (classType) {
            MonoObject* object = mono_object_new(Domain, classType);
            mono_runtime_object_init(object); // call ctor

            std::array<void*, 1> params { object };
         
            MonoObject* obj = mono_runtime_invoke(method, NULL, params.data(), NULL);
        }
    }
    else
    {
        LOG_ERROR("Failed to find extension in assembly.");
    }
}

void CustomMono::ReloadScripts() noexcept
{
    auto gatherScriptsInPath = [&](const std::filesystem::path& path) {
        std::error_code ec;
        auto iterator = std::filesystem::directory_iterator(path, ec);
        for (auto it = std::filesystem::begin(iterator); it != std::filesystem::end(iterator); it++) {
            auto filename = it->path().filename().u8string();
            auto name = it->path().filename();
            name.replace_extension("");

            auto extension = it->path().extension().u8string();
            if (!filename.empty() && extension == ".cs") {
                MonoScript script;
                script.name = std::move(it->path().filename().u8string());
                script.path = std::move(it->path().u8string());
                auto assembly = it->path(); assembly.replace_extension(".dll");
                script.assembly = assembly.u8string();
                Scripts.emplace_back(std::move(script));
            }
            else if (!filename.empty() && extension == ".dll") {
                MonoScript script;
                script.name = std::move(it->path().filename().u8string());
                script.assembly = it->path().u8string();
                Scripts.emplace_back(std::move(script));
            }
        }
    };
    Scripts.clear();
    gatherScriptsInPath(Util::Prefpath("mono/"));
}

void CustomMono::DrawUI() noexcept
{
    if (Domain == nullptr)
    {
        ImGui::TextUnformatted("Couldn't find mono.");
        ImGui::TextUnformatted("Please install mono 64-bit from mono-project.com");
        auto app = OpenFunscripter::ptr;
        if (ImGui::Button("Choose mono path"))
        {
            Util::OpenDirectoryDialog("Choose mono path", app->settings->data().monoSettings.MonoPath,
                [&](auto& result) {
                    if (result.files.size() > 0) {
                        OpenFunscripter::ptr->settings->data().monoSettings.MonoPath = result.files.front();
                        if (!Util::StringEndswith(result.files.front(), "\\") && !Util::StringEndswith(result.files.front(), "/")) {
                            OpenFunscripter::ptr->settings->data().monoSettings.MonoPath += "\\";
                        }
                        InitMono();
                    }
                });
        }
        ImGui::SameLine(); 
        ImGui::TextUnformatted(app->settings->data().monoSettings.MonoPath.c_str());
    }
    else
    {
        ImGui::Text("Mono: %s", mono_get_runtime_build_info());
        if (ImGui::Button("Reload", ImVec2(-1.f, 0.f))) {
            ReloadScripts();
        }
        ImGui::Separator();
        for (auto& script : Scripts) {
            if (script.path.empty()) {
                if (ImGui::Button(script.name.c_str(), ImVec2(-1.f, 0.f))) {
                    if (Util::FileExists(script.assembly))
                    {
                        OpenFunscripter::ptr->undoSystem->Snapshot(StateType::CUSTOM_MONO, true, OpenFunscripter::ptr->ActiveFunscript().get());
                        RunExtension(script.assembly);
                    }
                }
            }
            else {
                if (ImGui::Button(script.name.c_str(), ImVec2(-1.f, 0.f))) {
                    auto& MonoBasePath = OpenFunscripter::ptr->settings->data().monoSettings.MonoPath;
                    std::stringstream ss;
                    ss << MonoBasePath << "bin\\mcs.bat";
                    auto compilerPath = ss.str();
                    ss.str("");

                    auto extensionSourcePath = Util::Prefpath("mono\\OFS_Internal\\OFS_ExtensionBase.cs");
                    ss << "-out:" << script.assembly;
                    auto outFile = ss.str();

                    std::array<const char*, 6> args =
                    {
                        compilerPath.c_str(),
                        "-t:library",
                        extensionSourcePath.c_str(),
                        script.path.c_str(),
                        outFile.c_str(),
                        nullptr
                    };

                    if (UserAssembly) {
                        if (Domain && Domain != mono_get_root_domain()) {
                            if (mono_domain_set(mono_get_root_domain(), false)) {
                                mono_domain_unload(Domain);
                            }
                        }
                        //mono_assemblies_init();
                        Domain = mono_domain_create_appdomain("OFS Mono", NULL);
                        mono_domain_set(Domain, false);
                        UserAssembly = nullptr;
                    }

                    reproc::options options = {};
                    reproc::process proc;
                    std::error_code ec = proc.start(args.data(), options);
                    CompilerOutput = "";
                    reproc::sink::string sink(CompilerOutput);
                    ec = reproc::drain(proc, sink, sink);


                    auto [exitCode, ecCode] = proc.wait(reproc::deadline);

                    if (exitCode == 0) {
                        if (Util::FileExists(script.assembly))
                        {
                            LOGF_DEBUG("Compiled: %s", script.assembly.c_str());
                            OpenFunscripter::ptr->undoSystem->Snapshot(StateType::CUSTOM_MONO, true, OpenFunscripter::ptr->ActiveFunscript().get());
                            RunExtension(script.assembly);
                        }
                    }
                }
            }
        }
        ImGui::Separator();
        ImGui::TextUnformatted(CompilerOutput.c_str());
    }
}
