// Builds the room geometry. Units are metres, the floor is y = 0, the window
// wall is at z = -4 and the room is open (cut away) towards +z and +x.
#include "Primitives.h"
#include "Scene.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <random>

namespace {
const glm::vec3 kWhitePaint{0.93f, 0.91f, 0.87f};
const glm::vec3 kOak{0.86f, 0.70f, 0.52f};
const glm::vec3 kWalnut{0.55f, 0.38f, 0.26f};
}  // namespace

void Scene::buildRoom() {
    SceneNode* r = m_root.get();

    // diorama base slab and wooden floor
    addBox(r, "base", {8.7f, 0.34f, 7.7f}, {0.0f, -0.23f, -0.5f}, mat({0.20f, 0.17f, 0.16f}, "", 0.1f, 8.0f));
    Material floor = mat({1, 1, 1}, "floor_wood.png", 0.35f, 48.0f);
    floor.worldUV = 0.42f;
    addBox(r, "floor", {8.4f, 0.06f, 7.4f}, {0.0f, -0.03f, -0.5f}, floor);

    // walls: the back wall is 4 boxes around the window opening
    // (opening x = -0.6..1.4, y = 0.95..2.55)
    Material wall = mat({1, 1, 1}, "wallpaper.png", 0.06f, 8.0f);
    wall.worldUV = 0.75f;
    addBox(r, "wall_back_left", {3.6f, 3.2f, 0.2f}, {-2.4f, 1.6f, -4.1f}, wall);
    addBox(r, "wall_back_right", {2.8f, 3.2f, 0.2f}, {2.8f, 1.6f, -4.1f}, wall);
    addBox(r, "wall_back_below", {2.0f, 0.95f, 0.2f}, {0.4f, 0.475f, -4.1f}, wall);
    addBox(r, "wall_back_above", {2.0f, 0.65f, 0.2f}, {0.4f, 2.875f, -4.1f}, wall);
    addBox(r, "wall_left", {0.2f, 3.2f, 7.4f}, {-4.1f, 1.6f, -0.5f}, wall);

    // skirting boards
    Material skirting = mat(kWhitePaint, "", 0.3f, 32.0f);
    addBox(r, "skirting_back", {7.9f, 0.12f, 0.03f}, {0.05f, 0.06f, -3.985f}, skirting);
    addBox(r, "skirting_left", {0.03f, 0.12f, 7.2f}, {-3.985f, 0.06f, -0.4f}, skirting);

    // invisible occluders: the diorama has no ceiling and no right wall, but the
    // sun must only enter through the window, so these only cast shadows
    SceneNode* ceiling = addBox(r, "ceiling_shadow", {8.8f, 0.2f, 7.8f}, {0.0f, 3.3f, -0.5f}, Material{});
    ceiling->shadowOnly = true;
    SceneNode* rightWall = addBox(r, "right_wall_shadow", {0.2f, 3.6f, 7.8f}, {4.3f, 1.6f, -0.5f}, Material{});
    rightWall->shadowOnly = true;

    m_bounds.min = {-4.0f, 0.0f, -4.0f};
    m_bounds.max = {4.2f, 3.2f, 3.2f};
}

void Scene::buildWindow() {
    SceneNode* w = m_root->add("window");
    Material frame = mat(kWhitePaint, "wood.png", 0.35f, 48.0f);
    const float cx = 0.4f, cy = 1.75f, z = -4.1f;
    addBox(w, "frame_top", {2.0f, 0.07f, 0.12f}, {cx, 2.515f, z}, frame);
    addBox(w, "frame_bottom", {2.0f, 0.07f, 0.12f}, {cx, 0.985f, z}, frame);
    addBox(w, "frame_left", {0.07f, 1.6f, 0.12f}, {-0.565f, cy, z}, frame);
    addBox(w, "frame_right", {0.07f, 1.6f, 0.12f}, {1.365f, cy, z}, frame);
    addBox(w, "mullion_v", {0.05f, 1.5f, 0.06f}, {cx, cy, z}, frame);
    addBox(w, "mullion_h", {1.9f, 0.05f, 0.06f}, {cx, cy + 0.12f, z}, frame);
    addBox(w, "sill", {2.3f, 0.05f, 0.32f}, {cx, 0.945f, -3.9f}, frame);

    // glass: transparent, very shiny, does not block the sun
    Material glass;
    glass.albedo = glm::vec3(0.75f, 0.85f, 0.95f);
    glass.specular = 1.2f;
    glass.shininess = 180.0f;
    glass.alpha = 0.10f;
    SceneNode* pane = w->add("glass", keep(prim::quad(1.9f, 1.5f)), glass, {cx, cy, z + 0.02f});
    pane->castShadow = false;

    // curtains and rod
    Material curtain = mat({0.55f, 0.66f, 0.52f}, "fabric.png", 0.05f, 8.0f);
    addBox(w, "curtain_left", {0.45f, 2.1f, 0.06f}, {-0.95f, 1.85f, -3.9f}, curtain, 1.5f);
    addBox(w, "curtain_right", {0.45f, 2.1f, 0.06f}, {1.75f, 1.85f, -3.9f}, curtain, 1.5f);
    SceneNode* rod = w->add("curtain_rod", keep(prim::cylinder(0.018f, 0.018f, 3.3f, 12)),
                            mat({0.25f, 0.22f, 0.20f}, "", 0.6f, 64.0f), {-1.25f, 2.93f, -3.88f});
    rod->rotationDeg.z = -90.0f;  // cylinder axis Y -> X

    // little succulent on the window sill: silhouette in the sun beam
    SceneNode* pot = w->add("sill_pot", keep(prim::cylinder(0.05f, 0.065f, 0.1f, 20)),
                            mat({0.78f, 0.45f, 0.32f}, "", 0.15f, 16.0f), {1.15f, 0.97f, -3.88f});
    const Mesh* leaf = keep(prim::sphere(1.0f, 16, 8));
    for (int i = 0; i < 5; ++i) {
        SceneNode* l = pot->add("sill_leaf", leaf, mat({0.42f, 0.62f, 0.38f}, "", 0.25f, 24.0f), {0, 0.13f, 0});
        l->rotationDeg = {35.0f, i * 72.0f, 0.0f};
        l->scale = {0.025f, 0.07f, 0.025f};
    }
}

void Scene::buildDesk() {
    SceneNode* desk = m_root->add("desk", nullptr, Material{}, {0.4f, 0.0f, -3.55f});
    Material wood = mat(kOak, "wood.png", 0.3f, 40.0f);
    addBox(desk, "desk_top", {2.0f, 0.05f, 0.75f}, {0, 0.735f, 0}, wood, 1.2f);
    for (int i = 0; i < 4; ++i) {
        float x = (i % 2 ? 0.93f : -0.93f), z = (i / 2 ? 0.32f : -0.32f);
        addBox(desk, "desk_leg", {0.05f, 0.71f, 0.05f}, {x, 0.355f, z}, wood, 1.2f);
    }
    // drawer unit under the right side
    addBox(desk, "desk_drawers", {0.5f, 0.45f, 0.65f}, {0.65f, 0.49f, 0.0f}, wood, 1.2f);
    addBox(desk, "drawer_knob", {0.08f, 0.02f, 0.02f}, {0.65f, 0.6f, 0.335f}, mat({0.2f, 0.2f, 0.2f}, "", 0.8f, 64.0f));

    // laptop: base -> hinge -> lid -> screen (parent-child chain)
    Material alu = mat({0.70f, 0.71f, 0.74f}, "", 0.9f, 96.0f);
    SceneNode* laptop = addBox(desk, "laptop_base", {0.36f, 0.018f, 0.25f}, {0.12f, 0.769f, 0.12f}, alu);
    laptop->rotationDeg.y = -8.0f;
    SceneNode* hinge = laptop->add("laptop_hinge", nullptr, Material{}, {0.0f, 0.009f, -0.125f});
    hinge->rotationDeg.x = -14.0f;
    addBox(hinge, "laptop_lid", {0.36f, 0.24f, 0.012f}, {0.0f, 0.12f, 0.0f}, alu);
    Material screen = mat({1, 1, 1}, "screen_code.png", 0.6f, 96.0f);
    screen.textureIsEmissive = true;
    screen.emissive = glm::vec3(1.1f);
    m_laptopScreen = hinge->add("laptop_screen", keep(prim::quad(0.33f, 0.21f)), screen, {0.0f, 0.125f, 0.0065f});
    m_laptopScreen->castShadow = false;

    // desk lamp hierarchy: base -> yaw joint -> shoulder -> arm1 -> elbow -> arm2 -> head -> shade+bulb
    Material lampMetal = mat({0.20f, 0.36f, 0.33f}, "", 0.7f, 80.0f);
    SceneNode* base = desk->add("lamp_base", keep(prim::cylinder(0.09f, 0.08f, 0.03f, 24)), lampMetal, {-0.72f, 0.76f, -0.02f});
    m_lampYaw = base->add("lamp_yaw", nullptr, Material{}, {0.0f, 0.03f, 0.0f});
    SceneNode* shoulder = m_lampYaw->add("lamp_shoulder");
    shoulder->rotationDeg.z = -20.0f;
    const Mesh* rodMesh = keep(prim::cylinder(0.012f, 0.012f, 0.42f, 12));
    shoulder->add("lamp_arm1", rodMesh, lampMetal);
    SceneNode* elbow = shoulder->add("lamp_elbow", keep(prim::sphere(0.022f, 16, 8)), lampMetal, {0.0f, 0.42f, 0.0f});
    elbow->rotationDeg.z = -75.0f;
    elbow->add("lamp_arm2", rodMesh, lampMetal);
    m_lampHead = elbow->add("lamp_head", keep(prim::sphere(0.026f, 16, 8)), lampMetal, {0.0f, 0.42f, 0.0f});
    SceneNode* shade = m_lampHead->add("lamp_shade", keep(prim::cylinder(0.035f, 0.11f, 0.15f, 28, false)), lampMetal);
    shade->rotationDeg.x = 180.0f;  // open end points along the head's -Y (light direction)
    Material bulb = mat({1.0f, 0.95f, 0.85f}, "", 0.0f, 1.0f);
    m_lampBulb = m_lampHead->add("lamp_bulb", keep(prim::sphere(0.032f, 16, 8)), bulb, {0.0f, -0.07f, 0.0f});
    m_lampBulb->castShadow = false;

    // mug of coffee, books, small cactus
    Material ceramic = mat({0.92f, 0.86f, 0.74f}, "", 0.6f, 64.0f);
    SceneNode* mug = desk->add("mug", keep(prim::cylinder(0.045f, 0.045f, 0.10f, 24)), ceramic, {0.42f, 0.76f, 0.17f});
    SceneNode* handle = mug->add("mug_handle", keep(prim::torus(0.03f, 0.008f, 20, 8)), ceramic, {0.05f, 0.05f, 0.0f});
    handle->rotationDeg.y = 0.0f;
    mug->add("coffee", keep(prim::cylinder(0.04f, 0.04f, 0.002f, 20)), mat({0.25f, 0.14f, 0.08f}, "", 0.8f, 96.0f), {0, 0.088f, 0});

    const glm::vec3 bookCols[] = {{0.36f, 0.45f, 0.62f}, {0.80f, 0.62f, 0.32f}, {0.62f, 0.28f, 0.26f}};
    for (int i = 0; i < 3; ++i) {
        SceneNode* b = addBox(desk, "desk_book", {0.30f - i * 0.03f, 0.045f, 0.22f - i * 0.01f},
                              {0.75f, 0.783f + i * 0.045f, -0.12f}, mat(bookCols[i], "book_spine.png", 0.15f, 16.0f));
        b->rotationDeg.y = i * 9.0f - 6.0f;
    }
    SceneNode* cactusPot = desk->add("cactus_pot", keep(prim::cylinder(0.04f, 0.05f, 0.08f, 20)),
                                     mat({0.80f, 0.80f, 0.78f}, "", 0.4f, 32.0f), {-0.35f, 0.76f, -0.25f});
    SceneNode* cactus = cactusPot->add("cactus", keep(prim::sphere(1.0f, 16, 10)), mat({0.35f, 0.55f, 0.34f}, "", 0.2f, 16.0f), {0, 0.12f, 0});
    cactus->scale = {0.035f, 0.07f, 0.035f};

    // chair (in front of the desk, facing the window)
    SceneNode* chair = m_root->add("chair", nullptr, Material{}, {0.45f, 0.0f, -2.75f});
    chair->rotationDeg.y = 12.0f;
    Material chairWood = mat(kWalnut, "wood.png", 0.3f, 40.0f);
    addBox(chair, "chair_seat", {0.48f, 0.05f, 0.46f}, {0, 0.46f, 0}, chairWood, 1.5f);
    addBox(chair, "chair_cushion", {0.42f, 0.05f, 0.40f}, {0, 0.51f, 0}, mat({0.85f, 0.70f, 0.45f}, "fabric.png", 0.05f, 8.0f), 2.0f);
    for (int i = 0; i < 4; ++i) {
        float x = (i % 2 ? 0.2f : -0.2f), z = (i / 2 ? 0.19f : -0.19f);
        addBox(chair, "chair_leg", {0.04f, 0.44f, 0.04f}, {x, 0.22f, z}, chairWood, 1.5f);
    }
    addBox(chair, "chair_back", {0.46f, 0.36f, 0.04f}, {0, 0.78f, 0.21f}, chairWood, 1.5f);
    addBox(chair, "chair_post_l", {0.04f, 0.4f, 0.04f}, {-0.2f, 0.66f, 0.21f}, chairWood, 1.5f);
    addBox(chair, "chair_post_r", {0.04f, 0.4f, 0.04f}, {0.2f, 0.66f, 0.21f}, chairWood, 1.5f);
}

void Scene::buildBed() {
    SceneNode* bed = m_root->add("bed", nullptr, Material{}, {-3.15f, 0.0f, -2.85f});
    Material frame = mat(kWalnut, "wood.png", 0.3f, 40.0f);
    addBox(bed, "bed_frame", {1.6f, 0.26f, 2.2f}, {0, 0.25f, 0}, frame, 1.2f);
    for (int i = 0; i < 4; ++i)
        addBox(bed, "bed_leg", {0.08f, 0.12f, 0.08f}, {i % 2 ? 0.72f : -0.72f, 0.06f, i / 2 ? 1.02f : -1.02f}, frame);
    addBox(bed, "headboard", {1.6f, 0.95f, 0.08f}, {0, 0.6f, -1.06f}, frame, 1.2f);
    addBox(bed, "mattress", {1.5f, 0.22f, 2.08f}, {0, 0.49f, 0.02f}, mat({0.95f, 0.94f, 0.92f}, "fabric.png", 0.05f, 8.0f), 2.0f);
    addBox(bed, "blanket", {1.58f, 0.27f, 1.46f}, {0, 0.495f, 0.38f}, mat({1, 1, 1}, "blanket.png", 0.04f, 8.0f), 1.6f);
    addBox(bed, "blanket_fold", {1.6f, 0.05f, 0.25f}, {0, 0.645f, -0.25f}, mat({0.93f, 0.86f, 0.72f}, "fabric.png", 0.04f, 8.0f), 2.0f);
    const Mesh* pillow = keep(prim::sphere(1.0f, 24, 12));
    Material pillowMat = mat({0.86f, 0.88f, 0.92f}, "fabric.png", 0.05f, 8.0f);
    for (int i = 0; i < 2; ++i) {
        SceneNode* p = bed->add("pillow", pillow, pillowMat, {i ? 0.36f : -0.36f, 0.68f, -0.78f});
        p->scale = {0.32f, 0.09f, 0.19f};
        p->rotationDeg.x = -12.0f;
    }

    // fairy lights: two sagging swags on the wall above the bed. One instanced
    // draw for all bulbs, each bulb also is a point light.
    const int perSwag = 8;
    const float xs[3] = {-3.92f, -2.5f, -1.05f};
    std::vector<InstanceData> bulbs;
    std::vector<glm::vec3> wirePts;
    const glm::vec3 bulbCols[] = {{1.0f, 0.75f, 0.45f}, {1.0f, 0.62f, 0.35f}, {1.0f, 0.82f, 0.55f}};
    for (int s = 0; s < 2; ++s) {
        for (int i = 0; i <= perSwag; ++i) {
            if (s == 1 && i == 0) continue;
            float t = float(i) / perSwag;
            float x = xs[s] + (xs[s + 1] - xs[s]) * t;
            float y = 2.62f - 0.26f * std::sin(glm::pi<float>() * t);  // simple sag curve
            glm::vec3 p(x, y, -3.96f);
            wirePts.push_back(p);
            if (i == 0 || i == perSwag) continue;  // no bulb at the hooks
            glm::vec3 b = p - glm::vec3(0, 0.035f, -0.02f);
            m_fairyPositions.push_back(b);
            InstanceData d;
            d.model = glm::scale(glm::translate(glm::mat4(1.0f), b), glm::vec3(0.022f, 0.03f, 0.022f));
            glm::vec3 c = bulbCols[bulbs.size() % 3];
            d.color = glm::vec4(glm::pow(c, glm::vec3(2.2f)), float(bulbs.size()) * 1.7f);  // a = twinkle phase
            bulbs.push_back(d);
        }
    }
    auto bulbMesh = prim::sphere(1.0f, 12, 8);
    bulbMesh->setInstances(bulbs);
    Material bulbMat = mat({1.0f, 0.95f, 0.85f}, "", 0.3f, 32.0f);
    m_fairyBulbs = m_root->add("fairy_bulbs", keep(std::move(bulbMesh)), bulbMat);
    m_fairyBulbs->instanced = true;
    m_fairyBulbs->castShadow = false;

    Material wire = mat({0.12f, 0.12f, 0.10f}, "", 0.3f, 32.0f);
    const Mesh* wireMesh = keep(prim::cylinder(0.004f, 0.004f, 1.0f, 6, false));
    for (size_t i = 0; i + 1 < wirePts.size(); ++i) {
        glm::vec3 a = wirePts[i], b = wirePts[i + 1], d = b - a;
        SceneNode* seg = m_root->add("fairy_wire", wireMesh, wire, a);
        seg->rotationDeg.z = glm::degrees(std::atan2(-d.x, d.y));
        seg->scale = {1.0f, glm::length(d), 1.0f};
        seg->castShadow = false;
    }
}

void Scene::buildShelves() {
    // bookshelf against the left wall, books are one instanced mesh
    SceneNode* shelf = m_root->add("bookshelf", nullptr, Material{}, {-3.79f, 0.0f, 1.5f});
    Material wood = mat({0.95f, 0.93f, 0.90f}, "wood.png", 0.25f, 32.0f);
    addBox(shelf, "shelf_side_a", {0.38f, 1.9f, 0.04f}, {0, 0.95f, -0.7f}, wood, 1.2f);
    addBox(shelf, "shelf_side_b", {0.38f, 1.9f, 0.04f}, {0, 0.95f, 0.7f}, wood, 1.2f);
    addBox(shelf, "shelf_back", {0.02f, 1.9f, 1.36f}, {-0.18f, 0.95f, 0}, wood, 1.2f);
    const float levels[] = {0.04f, 0.5f, 0.96f, 1.42f, 1.885f};
    for (float y : levels) addBox(shelf, "shelf_board", {0.38f, 0.03f, 1.36f}, {0, y, 0}, wood, 1.2f);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> U(0.0f, 1.0f);
    const glm::vec3 palette[] = {{0.62f, 0.28f, 0.26f}, {0.30f, 0.42f, 0.58f}, {0.82f, 0.66f, 0.36f},
                                 {0.36f, 0.52f, 0.40f}, {0.55f, 0.40f, 0.58f}, {0.88f, 0.84f, 0.76f}};
    std::vector<InstanceData> books;
    for (int lvl = 0; lvl < 4; ++lvl) {
        float z = -0.64f;
        float end = (lvl == 2) ? 0.15f : 0.64f;  // leave space for decor on one shelf
        while (z < end) {
            float w = 0.03f + 0.035f * U(rng), h = 0.24f + 0.12f * U(rng), d = 0.2f + 0.08f * U(rng);
            if (z + w > end) break;
            bool lean = U(rng) < 0.08f;
            glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(-3.79f + 0.02f, levels[lvl] + 0.015f, 1.5f + z + w * 0.5f));
            if (lean) m = glm::rotate(m, glm::radians(12.0f), glm::vec3(1, 0, 0));
            m = glm::translate(m, glm::vec3(0, h * 0.5f, 0));
            d = glm::min(d, 0.3f);
            InstanceData inst;
            inst.model = glm::scale(m, glm::vec3(d, h, w));
            glm::vec3 c = palette[int(U(rng) * 6) % 6] * (0.85f + 0.3f * U(rng));
            inst.color = glm::vec4(glm::pow(c, glm::vec3(2.2f)), 0.0f);
            books.push_back(inst);
            z += w + 0.004f;
        }
    }
    auto bookMesh = prim::box({1, 1, 1});
    bookMesh->setInstances(books);
    SceneNode* bookNode = m_root->add("books", keep(std::move(bookMesh)), mat({1, 1, 1}, "book_spine.png", 0.15f, 16.0f));
    bookNode->instanced = true;

    // a globe on the shelf with the gap: stand -> axis tilt -> sphere
    SceneNode* globeStand = shelf->add("globe_stand", keep(prim::cylinder(0.06f, 0.03f, 0.05f, 20)),
                                       mat({0.3f, 0.25f, 0.2f}, "", 0.5f, 48.0f), {0.02f, 0.975f, 0.42f});
    SceneNode* tilt = globeStand->add("globe_tilt", nullptr, Material{}, {0, 0.15f, 0});
    tilt->rotationDeg.z = 23.4f;
    tilt->add("globe", keep(prim::sphere(0.1f, 24, 12)), mat({0.35f, 0.55f, 0.75f}, "", 0.5f, 48.0f));

    // wall shelf above the bed with a plant and a candle jar
    SceneNode* wallShelf = addBox(m_root.get(), "wall_shelf", {0.24f, 0.035f, 1.0f}, {-3.88f, 1.72f, -1.75f}, mat(kOak, "wood.png", 0.3f, 32.0f), 1.5f);
    SceneNode* pot = wallShelf->add("hanging_pot", keep(prim::cylinder(0.06f, 0.075f, 0.12f, 20)),
                                    mat({0.92f, 0.90f, 0.86f}, "", 0.4f, 32.0f), {0.0f, 0.018f, 0.3f});
    const Mesh* leafMesh = keep(prim::sphere(1.0f, 12, 8));
    Material leaf = mat({0.35f, 0.58f, 0.32f}, "", 0.3f, 24.0f);
    for (int i = 0; i < 9; ++i) {  // trailing pothos: leaves hanging down the front
        SceneNode* l = pot->add("pothos_leaf", leafMesh, leaf,
                                {0.06f + 0.02f * (i % 3), 0.1f - 0.11f * i, -0.06f + 0.05f * std::sin(i * 1.9f)});
        l->scale = {0.02f, 0.045f, 0.035f};
        l->rotationDeg = {0.0f, 20.0f * i, 30.0f};
    }
    for (int i = 0; i < 3; ++i)
        addBox(wallShelf, "shelf_book", {0.17f, 0.2f, 0.04f}, {0.0f, 0.118f, -0.4f + i * 0.045f},
               mat(palette[i + 2], "book_spine.png", 0.15f, 16.0f));
}

void Scene::buildDecor() {
    SceneNode* r = m_root.get();
    // rug (thin box, texture covers it once)
    SceneNode* rug = addBox(r, "rug", {3.2f, 0.012f, 2.4f}, {0.1f, 0.006f, -1.0f}, mat({1, 1, 1}, "rug.png", 0.03f, 4.0f));
    rug->castShadow = false;

    // poster on the left wall
    SceneNode* poster = r->add("poster", keep(prim::quad(0.78f, 1.1f)), mat({1, 1, 1}, "poster.png", 0.15f, 24.0f),
                               {-3.995f, 1.75f, -0.15f});
    poster->rotationDeg.y = 90.0f;
    poster->castShadow = false;

    // pouf on the rug
    SceneNode* pouf = r->add("pouf", keep(prim::sphere(1.0f, 32, 16)), mat({0.86f, 0.62f, 0.30f}, "fabric.png", 0.05f, 8.0f),
                             {1.9f, 0.24f, -0.55f});
    pouf->scale = {0.42f, 0.26f, 0.42f};

    // floor lamp: base -> pole -> shade (+ bulb = point light)
    SceneNode* lamp = r->add("floor_lamp", keep(prim::cylinder(0.17f, 0.15f, 0.03f, 28)),
                             mat({0.15f, 0.14f, 0.13f}, "", 0.6f, 64.0f), {-3.35f, 0.0f, -1.05f});
    lamp->add("floor_lamp_pole", keep(prim::cylinder(0.014f, 0.014f, 1.5f, 12)), mat({0.75f, 0.62f, 0.38f}, "", 0.8f, 96.0f));
    m_floorShade = lamp->add("floor_lamp_shade", keep(prim::cylinder(0.25f, 0.17f, 0.3f, 32, false)),
                             mat({0.95f, 0.90f, 0.80f}, "fabric.png", 0.05f, 8.0f), {0.0f, 1.42f, 0.0f});
    m_floorShade->castShadow = false;
    m_floorBulb = lamp->add("floor_lamp_bulb", keep(prim::sphere(0.045f, 16, 8)), mat({1.0f, 0.95f, 0.85f}, "", 0.0f, 1.0f),
                            {0.0f, 1.55f, 0.0f});
    m_floorBulb->castShadow = false;

    // big monstera-like plant in the right corner
    SceneNode* pot = r->add("plant_pot", keep(prim::cylinder(0.17f, 0.23f, 0.40f, 28)),
                            mat({0.78f, 0.47f, 0.33f}, "", 0.2f, 16.0f), {3.45f, 0.0f, -1.7f});
    pot->add("soil", keep(prim::cylinder(0.21f, 0.21f, 0.01f, 24)), mat({0.20f, 0.14f, 0.10f}, "", 0.05f, 4.0f), {0, 0.37f, 0});
    const Mesh* leaf = keep(prim::sphere(1.0f, 20, 10));
    const Mesh* stem = keep(prim::cylinder(0.008f, 0.008f, 1.0f, 8, false));
    Material leafMat = mat({0.24f, 0.48f, 0.27f}, "", 0.45f, 40.0f);
    for (int i = 0; i < 10; ++i) {
        SceneNode* s = pot->add("plant_stem", nullptr, Material{}, {0, 0.38f, 0});
        s->rotationDeg = {20.0f + 11.0f * (i % 3), i * 36.0f + 10.0f, 0.0f};
        float len = 0.55f + 0.12f * (i % 4);
        SceneNode* st = s->add("stem_mesh", stem, leafMat);
        st->scale = {1.0f, len, 1.0f};
        SceneNode* l = s->add("plant_leaf", leaf, leafMat, {0, len + 0.05f, 0.06f});
        l->scale = {0.09f, 0.012f, 0.17f};
        l->rotationDeg.x = 35.0f;
    }
}

void Scene::buildRecordCorner() {
    SceneNode* r = m_root.get();
    Material wood = mat(kWalnut, "wood.png", 0.3f, 40.0f);
    SceneNode* cabinet = addBox(r, "cabinet", {1.4f, 0.6f, 0.45f}, {2.95f, 0.3f, -3.74f}, wood, 1.2f);
    addBox(cabinet, "cabinet_line", {1.36f, 0.01f, 0.005f}, {0, 0.0f, 0.226f}, mat({0.15f, 0.1f, 0.08f}));
    for (int i = 0; i < 2; ++i)
        addBox(cabinet, "cabinet_knob", {0.1f, 0.02f, 0.02f}, {i ? 0.35f : -0.35f, 0.12f, 0.235f}, mat({0.75f, 0.62f, 0.38f}, "", 0.8f, 96.0f));

    // record player: base -> platter (spins) -> vinyl ; base -> tonearm pivot (swings)
    SceneNode* player = addBox(cabinet, "record_player", {0.46f, 0.08f, 0.36f}, {-0.25f, 0.34f, 0.0f},
                               mat({0.85f, 0.80f, 0.72f}, "wood.png", 0.35f, 48.0f), 2.0f);
    m_platter = player->add("platter", keep(prim::cylinder(0.155f, 0.155f, 0.012f, 40)), mat({0.15f, 0.15f, 0.16f}, "", 0.6f, 64.0f),
                            {-0.05f, 0.04f, 0.0f});
    m_platter->add("vinyl", keep(prim::cylinder(0.148f, 0.148f, 0.004f, 48)), mat({1, 1, 1}, "vinyl.png", 0.9f, 120.0f), {0, 0.012f, 0});
    SceneNode* pivot = player->add("tonearm_pivot", keep(prim::cylinder(0.018f, 0.018f, 0.04f, 16)),
                                   mat({0.8f, 0.8f, 0.82f}, "", 0.9f, 96.0f), {0.17f, 0.04f, -0.12f});
    m_tonearm = pivot->add("tonearm", nullptr, Material{}, {0.0f, 0.035f, 0.0f});
    addBox(m_tonearm, "tonearm_bar", {0.012f, 0.012f, 0.24f}, {0.0f, 0.0f, 0.12f}, mat({0.8f, 0.8f, 0.82f}, "", 0.9f, 96.0f));
    addBox(m_tonearm, "tonearm_head", {0.03f, 0.015f, 0.04f}, {-0.01f, -0.006f, 0.245f}, mat({0.1f, 0.1f, 0.1f}, "", 0.5f, 48.0f));

    // speaker
    SceneNode* speaker = addBox(cabinet, "speaker", {0.26f, 0.38f, 0.24f}, {0.45f, 0.49f, 0.0f}, wood, 2.0f);
    SceneNode* woofer = speaker->add("woofer", keep(prim::cylinder(0.08f, 0.08f, 0.01f, 32)),
                                     mat({0.08f, 0.08f, 0.08f}, "", 0.3f, 24.0f), {0, -0.05f, 0.12f});
    woofer->rotationDeg.x = 90.0f;
    SceneNode* tweeter = speaker->add("tweeter", keep(prim::cylinder(0.03f, 0.03f, 0.01f, 24)),
                                      mat({0.08f, 0.08f, 0.08f}, "", 0.3f, 24.0f), {0, 0.11f, 0.12f});
    tweeter->rotationDeg.x = 90.0f;

    // wall clock: body -> hour hand / minute hand (rotate with the time of day)
    SceneNode* clock = r->add("clock", nullptr, Material{}, {2.95f, 2.15f, -3.975f});
    SceneNode* body = clock->add("clock_body", keep(prim::cylinder(0.23f, 0.23f, 0.04f, 48)),
                                 mat({1, 1, 1}, "clock_face.png", 0.3f, 32.0f));
    body->rotationDeg.x = 90.0f;  // disc axis Y -> Z, face looks into the room
    clock->add("clock_rim", keep(prim::torus(0.23f, 0.018f, 48, 10)), mat({0.20f, 0.18f, 0.16f}, "", 0.6f, 64.0f), {0, 0, 0.03f});
    Material handMat = mat({0.12f, 0.12f, 0.14f}, "", 0.4f, 32.0f);
    m_hourHand = clock->add("hour_hand", nullptr, Material{}, {0, 0, 0.048f});
    addBox(m_hourHand, "hour_hand_mesh", {0.022f, 0.12f, 0.006f}, {0, 0.05f, 0}, handMat);
    m_minuteHand = clock->add("minute_hand", nullptr, Material{}, {0, 0, 0.054f});
    addBox(m_minuteHand, "minute_hand_mesh", {0.014f, 0.18f, 0.006f}, {0, 0.08f, 0}, handMat);
}
