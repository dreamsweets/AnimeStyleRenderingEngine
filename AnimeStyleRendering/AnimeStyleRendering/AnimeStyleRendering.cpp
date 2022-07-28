#include "AnimeStyleRendering.h"
#include "imgui/IMGUIManager.h"

#include "Resources/ResourceManager.h"
#include "Render/RenderManager.h"
#include "UI/ContentsBrowser.h"

#include "Components/Primitive/MeshRendererComponent.h"
#include "Components/Primitive/RayTracingMeshComponent.h"
#include "Components/Camera/CameraComponent.h"
#include "Components/Light/LightComponent.h"
#include "Components/Script/CameraMoveScript.h"
#include "Components/Primitive/AnimatorComponent.h"
#include "Components/Primitive/SkyComponent.h"

#include "DX12/Gfx.h"
#include "DX12/Material.h"
#include "DX12/VertexBuffer.h"
#include "DX12/IndexBuffer.h"
#include "DX12/RayTracingMaterial.h"
#include "DX12/ComputeShader.h"

#include "Resources/Animation/AnimationClip.h"
#include "Resources/Sky/Sky.h"

#include "UI/TextureViewer.h"

using namespace DirectX;

IMPLEMENT_SINGLE(AnimeStyleRendering)

void AnimeStyleRendering::Init()
{
    {
        RenderManager::Inst().Init();
        Input::Inst().Init();
        ResourceManager::Inst().Init();
        Time::Init();
        IMGUIManager::Inst().AddWindow<ContentsBrowser>();
    }
    
    {
        m_CurrentScene = new Scene;
    }
   
	CreateAssets();
    m_CurrentScene ? m_CurrentScene->Start() : (void)0;
}

void AnimeStyleRendering::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        else {
            Update();
            Render();
            PostUpdate();
            Time::Tick();
        }
    }
}

void AnimeStyleRendering::Destroy()
{
    Gfx::Destroy();
}

struct Vertex {
    math::vec3 position;
    math::vec4 color;
};

void AnimeStyleRendering::CreateAssets()
{
    //Camera
    {
        auto e = m_CurrentScene->AddEntity();
        e->Name("MainCamera");
        e->AddComponent<CameraComponent>();
        e->GetComponent<TransformComponent>()->Position({ 0.67, 3.87, -1.77 });
        e->GetComponent<TransformComponent>()->Rotation({ 11.3, -14.64, 0});
        e->AddComponent<CameraMoveScript>();
    }
    //Directional Light
    {
        auto e = m_CurrentScene->AddEntity();
        e->Name("Directional Light");
        e->GetTransform()->Rotation({34, 51, -12});
        auto light = e->AddComponent<DirectionalLightComponent>();
        light->SetColor({1,1,1});
        light->SetCastShadow(true);
    }

    //Point Light(10 pieces)
    {
        auto e = m_CurrentScene->AddEntity();
        e->Name("Blue Point light");
        e->GetTransform()->Position({ -2.5, 0, -2.5 });
        auto light = e->AddComponent<PointLightComponent>();
        light->SetColor({ 0,0,1 });
        light->SetRange(5);

        e = m_CurrentScene->AddEntity();
        e->Name("Red Point light");
        e->GetTransform()->Position({ 2.9, 1.1, 2.6 });
        light = e->AddComponent<PointLightComponent>();
        light->SetColor({ 1,0,0 });
        light->SetRange(5);
    }


    {
        auto Testcharacter = ResourceManager::Inst().GetModel("DancingBox.fbx");
        AnimationClip* clip = ResourceManager::Inst().LoadAnimationClip("Assets/DancingBox.fbx");
        clip->SetSkeleton(Testcharacter->GetSkeleton());
        Animation* animation = ResourceManager::Inst().CreateAnimation("Test");
        animation->SetSkeleton(Testcharacter->GetSkeleton());
        animation->AddAnimationClip("Test", clip);

        auto modelEntity = m_CurrentScene->AddEntity();

        modelEntity->Name(Testcharacter->GetName());
        modelEntity->GetTransform()->Position({ 0, 2.8, 0 });
        modelEntity->GetTransform()->Rotation({ 90, 0, 0 });

        auto animcomponent = modelEntity->AddComponent<AnimatorComponent>();
        animcomponent->SetAnimation(ResourceManager::Inst().GetAnimation("Test"));
        
        std::vector<Entity*> skeletonEntities;

        auto skeletonData = Testcharacter->GetSkeleton();
        modelEntity->SetChild(skeletonData->CreateBoneAsEntity(m_CurrentScene));
        std::vector<std::shared_ptr<MeshRendererComponent>> meshrenderers;
        for (auto& mesh : Testcharacter->GetVecMesh())
        {
            auto entity = m_CurrentScene->AddEntity(false);
            modelEntity->SetChild(entity);
            entity->Name(mesh->GetName());
            auto component = entity->AddComponent<MeshRendererComponent>();
            meshrenderers.push_back(component);
            component->SetMesh(mesh);
            auto rtcomponent = entity->AddComponent<RayTracingMeshComponent>();
            rtcomponent->AddAnimation(animation);
            rtcomponent->AddMesh(mesh, true);

            Material* geoMaterial = new GeometryMaterial;
            geoMaterial->SetShader(ResourceManager::Inst().GetShader("AnimationMeshGeometryPassShader.fx"));
            component->SetMaterial(geoMaterial);
            component->GetMaterial(0)->SetTexture("Albedo", ResourceManager::Inst().GetTexture("utc_all2_outlinesmpler.png"));
        }
        animcomponent->AddMeshRenderer(meshrenderers);
    }
    
    //Character Mesh ·Îµå
    {
        auto character = ResourceManager::Inst().GetModel("SD_unitychan.fbx");
        
        auto modelEntity = m_CurrentScene->AddEntity();

        modelEntity->Name(character->GetName());
        modelEntity->GetTransform()->Position({0, 2.8, 0});
        modelEntity->GetTransform()->Rotation({ 90, 0, 0 });

        for (auto& mesh : character->GetVecMesh())
        {
            auto entity = m_CurrentScene->AddEntity(false);
            modelEntity->SetChild(entity);
            entity->Name(mesh->GetName());
            auto component = entity->AddComponent<MeshRendererComponent>();
            component->SetMesh(mesh);

            auto rtcomponent = entity->AddComponent<RayTracingMeshComponent>();
            rtcomponent->AddMesh(mesh);
             
            //Material* outlineMaterial = new Material;
            //outlineMaterial->SetShader(outlineShader);
            //outlineMaterial->EnableDepth(false);
            //outlineMaterial->SetCullMode(D3D12_CULL_MODE_FRONT);
            //OutlineInfo outline{};
            //outlineMaterial->SetCBufferValue("OutlineInfo", outline);
            //component->SetMaterial(outlineMaterial);

            Material* geoMaterial = new GeometryMaterial;
            geoMaterial->SetShader(ResourceManager::Inst().GetShader("GeometryPassShader.fx"));
            component->SetMaterial(geoMaterial);
            component->GetMaterial(0)->SetTexture("Albedo", ResourceManager::Inst().GetTexture("utc_all2_light.png"));
        }
    }

    {
        auto model = ResourceManager::Inst().GetModel("TestScene.fbx");

        auto modelEntity = m_CurrentScene->AddEntity();
        modelEntity->GetTransform()->Rotation({ 90, 0, 0 });
        modelEntity->GetTransform()->Scale({ 0.1, 0.1, 0.1 });
        modelEntity->Name(model->GetName());
        for (auto& mesh : model->GetVecMesh())
        {
            auto entity = m_CurrentScene->AddEntity(false);
            modelEntity->SetChild(entity);
            entity->Name(mesh->GetName());
            auto component = entity->AddComponent<MeshRendererComponent>();
            component->SetMesh(mesh);

            auto rtcomponent = entity->AddComponent<RayTracingMeshComponent>();
            rtcomponent->AddMesh(mesh);

            //Material* outlineMaterial = new Material;
            //outlineMaterial->SetShader(outlineShader);
            //outlineMaterial->EnableDepth(false);
            //outlineMaterial->SetCullMode(D3D12_CULL_MODE_FRONT);
            //OutlineInfo outline{};
            //outlineMaterial->SetCBufferValue("OutlineInfo", outline);
            //component->SetMaterial(outlineMaterial);

            Material* geoMaterial = new GeometryMaterial;
            geoMaterial->SetShader(ResourceManager::Inst().GetShader("GeometryPassShader.fx"));
            component->SetMaterial(geoMaterial);
            component->GetMaterial(0)->SetTexture("Albedo", ResourceManager::Inst().GetTexture("Dummy.jpg"));
        }
    }

    {
        auto model = ResourceManager::Inst().GetModel("sphere.fbx");

        auto modelEntity = m_CurrentScene->AddEntity();
        modelEntity->GetTransform()->Rotation({ 90, 0, 0 });
        modelEntity->GetTransform()->Scale({ 0.1, 0.1, 0.1 });
        modelEntity->Name(model->GetName());
        for (auto& mesh : model->GetVecMesh())
        {
            auto entity = m_CurrentScene->AddEntity(false);
            modelEntity->SetChild(entity);
            entity->Name(mesh->GetName());
            auto component = entity->AddComponent<MeshRendererComponent>();
            component->SetMesh(mesh);

            auto rtcomponent = entity->AddComponent<RayTracingMeshComponent>();
            rtcomponent->AddMesh(mesh);

            //Material* outlineMaterial = new Material;
            //outlineMaterial->SetShader(outlineShader);
            //outlineMaterial->EnableDepth(false);
            //outlineMaterial->SetCullMode(D3D12_CULL_MODE_FRONT);
            //OutlineInfo outline{};
            //outlineMaterial->SetCBufferValue("OutlineInfo", outline);
            //component->SetMaterial(outlineMaterial);

            Material* geoMaterial = new GeometryMaterial;
            geoMaterial->SetShader(ResourceManager::Inst().GetShader("GeometryPassShader.fx"));
            component->SetMaterial(geoMaterial);
            component->GetMaterial(0)->SetTexture("Albedo", ResourceManager::Inst().GetTexture("Dummy.jpg"));
        }
    }

    {

        auto modelEntity = m_CurrentScene->AddEntity();
        modelEntity->Name("Sky");
        auto component = modelEntity->AddComponent<SkyComponent>();
        modelEntity->GetTransform()->Scale({100, 100, 100});
        Material* geoMaterial = new Material;
        geoMaterial->SetShader(ResourceManager::Inst().GetShader("SkyShader.fx"));
        geoMaterial->SetCullMode(D3D12_CULL_MODE_FRONT);
        geoMaterial->EnableDepth(false);

        component->SetMaterial(geoMaterial);
    }
}

void AnimeStyleRendering::Update()
{
    Input::Inst().Update();
    m_CurrentScene ? m_CurrentScene->Update() : (void)0;
    RenderManager::Inst().Update();
}

void AnimeStyleRendering::PostUpdate()
{
    Input::Inst().PostUpdate();
    m_CurrentScene ? m_CurrentScene->PostUpdate() : (void)0;
}

void AnimeStyleRendering::Render()
{
    RenderManager::Inst().Render();
    RenderManager::Inst().PostRender();
}