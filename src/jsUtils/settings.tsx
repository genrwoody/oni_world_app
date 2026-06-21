import React from "react";
import { useState } from "react";
import Form from "react-bootstrap/Form";

import configuration from "./configuration";
import useTranslation from "./language";
import Mixings from "./mixings";

interface SettingsProps {
    cluster: number;
    mixings: number;
    traits: string;
    onChange: (cluster: number, mixings: number, traits: string) => void;
}

interface TraitItemProps {
    options: string;
    value: string;
    onChange: (value: string) => void;
}

const TraitItem = ({ options, value, onChange }: TraitItemProps) => {
    const translation = useTranslation();
    const getTraitName = (key: string): string => {
        return configuration.traits.find((trait) => trait.key == key)!.name;
    };
    return (
        <Form.Select
            className="mb-3"
            value={value}
            onChange={(e) => onChange(e.target.value)}
        >
            {Array.from(options).map((item, index) => (
                <option key={index} value={item}>
                    {translation(getTraitName(item))}
                </option>
            ))}
        </Form.Select>
    );
};

const Settings = ({ cluster, mixings, traits, onChange }: SettingsProps) => {
    const getTraits = (index: number) => {
        const cluster = configuration.cluster[index];
        return Array<string>(cluster.max).fill(cluster.traits);
    };
    const filterTraits = (traits: string, enable: string) => {
        traits = traits.replace(enable, "");
        if (enable === "F" || enable === "H") {
            // geo active/dorment
            traits = traits.replace(/[FH]/, "");
        }
        if (enable === "4" || enable === "E") {
            // core traits
            traits = traits.replace(/[4E]/, "");
        }
        if (enable === "L" || enable === "M") {
            // metal rich/poor
            traits = traits.replace(/[LM]/, "");
        }
        return traits;
    };
    const fillTraitOptions = (traits: Array<string>) => {
        return getTraits(cluster).map((item, index1) => {
            traits.forEach((trait, index2) => {
                if (trait !== "Z" && index1 !== index2) {
                    item = filterTraits(item, trait);
                }
            });
            return item;
        });
    };
    const initOptions = fillTraitOptions(traits.split(""));
    const [traitOptions, setTraitOptions] = useState(initOptions);
    const translation = useTranslation();
    const onCategoryChange = (value: number) => {
        const cluster = value === 0 ? 0 : value === 1 ? 14 : 29;
        setTraitOptions(getTraits(cluster));
        onChange(cluster, mixings, "ZZZZ");
    };
    const onClusterChange = (value: number) => {
        setTraitOptions(getTraits(value));
        onChange(value, mixings, "ZZZZ");
    };
    const onMixingsChange = (value: number) => {
        onChange(cluster, value, traits);
    };
    const onTraitsChange = (enable: string, index: number) => {
        const traitsArray = traits.split("");
        traitsArray[index] = enable;
        const options = fillTraitOptions(traitsArray);
        setTraitOptions(options);
        onChange(cluster, mixings, traitsArray.join(""));
    };
    const labels = ["Asteroid", "Planetoid Cluster", "Moonlet Cluster"];
    const clusterType = configuration.cluster[cluster].type;
    return (
        <Form>
            <Form.Group className="mb-3" controlId="mode">
                <Form.Label>{translation("Game Mode")}</Form.Label>
                <Form.Select
                    aria-label="Game Mode"
                    defaultValue={clusterType}
                    onChange={(e) => {
                        onCategoryChange(parseInt(e.target.value));
                    }}
                >
                    {configuration.game.map((item, index) => (
                        <option key={index} value={index}>
                            {translation(item.name)}
                        </option>
                    ))}
                </Form.Select>
            </Form.Group>
            <Form.Group className="mb-3" controlId="cluster">
                <Form.Label>{translation(labels[clusterType])}</Form.Label>
                <Form.Select
                    aria-label={configuration.cluster[cluster].name}
                    value={cluster}
                    onChange={(e) => {
                        onClusterChange(parseInt(e.target.value));
                    }}
                >
                    {configuration.cluster
                        .map((item, index) => ({ index: index, ...item }))
                        .filter((item) => item.type == clusterType)
                        .map((item, index) => (
                            <option key={index} value={item.index}>
                                {translation(item.name)}
                            </option>
                        ))}
                    ;
                </Form.Select>
            </Form.Group>
            <Form.Group className="mb-3">
                <Form.Label>{translation("World Traits")}</Form.Label>
                {traitOptions.map((item, index) => (
                    <TraitItem
                        key={index}
                        value={traits.at(index) || "Z"}
                        options={item}
                        onChange={(value) => onTraitsChange(value, index)}
                    />
                ))}
            </Form.Group>
            <Form.Group className="mb-3">
                <Form.Label>{translation("Scramble DLCs")}</Form.Label>
            </Form.Group>
            {configuration.mixing
                .filter((item) => item.key[2] === "p")
                .map((item, index) => (
                    <Mixings
                        key={index}
                        index={clusterType}
                        name={item.key}
                        active={mixings}
                        onSetActive={(active) => {
                            onMixingsChange(active);
                        }}
                    />
                ))}
        </Form>
    );
};

export default Settings;
