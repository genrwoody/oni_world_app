#pragma once

enum class TagCommand {
    Default, //
    AtTag,
    NotAtTag,
    DistanceFromTag
};

enum class Command {
    Clear,
    Replace,
    UnionWith,
    IntersectWith,
    ExceptWith,
    SymmetricExceptWith,
    All
};

enum class ListRule {
    GuaranteeOne,
    GuaranteeSome,
    GuaranteeSomeTryMore,
    GuaranteeAll,
    GuaranteeRange,
    TryOne,
    TrySome,
    Trysome = TrySome,
    TryRange,
    TryAll
};

enum class OrderRule {
    Prepend, //
    Append,
    Replace,
    Invalid
};

enum class ZoneType {
    FrozenWastes,
    CrystalCaverns,
    BoggyMarsh,
    Sandstone,
    ToxicJungle,
    MagmaCore,
    OilField,
    Space,
    Ocean,
    Rust,
    Forest,
    Radioactive,
    Swamp,
    Wasteland,
    RocketInterior,
    Metallic,
    Barren,
    Moo,
    IceCaves,
    CarrotQuarry,
    SugarWoods,
    PrehistoricGarden,
    PrehistoricRaptor,
    PrehistoricWetlands
};

enum class LocationType {
    Cluster, //
    StartWorld,
    InnerCluster
};

enum class WorldCategory {
    Asteroid, //
    Moon
};

enum class Skip {
    Never = 0,
    False = Never,
    Always = 99,
    True = Always,
    EditorOnly = 100
};

enum class LayoutMethod {
    Default = 0, //
    VoronoiTree = Default,
    PowerTree = 1
};

enum class SampleBehaviour {
    UniformSquare,
    UniformHex,
    UniformScaledHex,
    UniformSpiral,
    UniformCircle,
    PoissonDisk,
    StdRand
};

enum class PointSelectionMethod {
    RandomPoints, //
    Centroid
};

enum class Location {
    Floor,
    Ceiling,
    Air,
    BackWall,
    NearWater,
    NearLiquid,
    Solid,
    Water,
    ShallowLiquid,
    Surface,
    LiquidFloor,
    AnyFloor,
    LiquidCeiling,
    Liquid,
    EntombedFloorPeek
};

enum class Shape {
    Circle,
    Oval,
    Blob,
    Line,
    Square,
    TallThin,
    ShortWide,
    Template,
    PhysicalLayout,
    Splat
};

enum class Selection {
    None,
    OneOfEach,
    NOfEach,
    Weighted,
    WeightedBucket,
    WeightedResample,
    PickOneWeighted,
    HorizontalSlice
};

enum class Range {
    ExtremelyCold,
    VeryVeryCold,
    VeryCold,
    Cold,
    Chilly,
    Cool,
    Mild,
    Room,
    HumanWarm,
    HumanHot,
    Hot,
    VeryHot,
    ExtremelyHot
};

enum class Type {
    Building, //
    Ore,
    Pickupable,
    Other
};

enum class Orientation {
    Neutral, //
    R90,
    R180,
    R270,
    NumRotations,
    FlipH,
    FlipV
};

enum class ClusterCategory {
    Vanilla,
    SpacedOutVanillaStyle,
    SpacedOutStyle,
    Special
};

enum class ClusterType {
    // Base Game
    Terra,
    // ignore lines
    Oceania,
    Rime,
    Verdante,
    Arboria,
    Volcanea,
    Badlands,
    Aridio,
    Oasisse,
    Ceres,
    Blasted_Ceres,
    Relica,
    RelicAAAAGH,
    // Classic
    Terra_Cluster,
    Oceania_Cluster,
    Squelchy_Cluster,
    Rime_Cluster,
    Verdante_Cluster,
    Arboria_Cluster,
    Volcanea_Cluster,
    Badlands_Cluster,
    Aridio_Cluster,
    Oasisse_Cluster,
    Ceres_Cluster,
    Blasted_Ceres_Cluster,
    Relica_Cluster,
    RelicAAAAGH_Cluster,
    // Space Out!
    Terrania_Cluster,
    Relica_Minor_Cluster,
    Ceres_Minor_Cluster,
    Folia_Cluster,
    Quagmiris_Cluster,
    Metallic_Swampy_Moonlet,
    Desolands_Moonlet,
    Frozen_Forest_Moonlet,
    Flipped_Moonlet,
    Radioactive_Ocean_Moonlet,
    Ceres_Mantle_Moonlet
};

