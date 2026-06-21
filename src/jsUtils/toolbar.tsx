import React from "react";
import { useState } from "react";
import InputGroup from "react-bootstrap/InputGroup";
import Button from "react-bootstrap/Button";
import Form from "react-bootstrap/Form";
import Offcanvas from "react-bootstrap/Offcanvas";
import Navbar from "react-bootstrap/Navbar";
import Container from "react-bootstrap/Container";

import configuration from "./configuration";
import useTranslation from "./language";
import Settings from "./settings";
import NavRight from "./navright";

interface ToolBarProps {
    onSetAppConfig: (lang: string, theme: number) => void;
    onSetWorld: () => void;
}

const ToolBar = ({ onSetAppConfig, onSetWorld }: ToolBarProps) => {
    const hexChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const [drawer, setDrawer] = useState(true);
    const [cluster, setCluster] = useState(0);
    const [traits, setTraits] = useState("ZZZZ");
    const [mixings, setMixings] = useState(152646487500);
    const [seed, setSeed] = useState("");
    const translation = useTranslation();
    const toBase36 = (num: number) => {
        if (num === 0) return "0";
        let text = "";
        while (num > 0) {
            text += hexChars[num % 36];
            num = Math.floor(num / 36);
        }
        return text;
    };
    const traitsToNumber = () => {
        let num = 0;
        traits.split("").forEach((item) => {
            if (item !== "0" && item !== "Z") {
                num |= 1 << (hexChars.indexOf(item) - 1);
            }
        });
        return -num;
    };
    const generateWorld = (nseed: number) => {
        Module.worlds.length = 0;
        Module.app_generate(cluster, nseed, mixings);
        setSeed(Module.worlds[0].seed.toString());
        Module.worlds.forEach(
            (item) => (item.coord = getSeedString(item.seed))
        );
        onSetWorld();
    };
    const getSeedString = (seed: number | string) => {
        const name = configuration.cluster[cluster].prefix;
        const mix = toBase36(mixings);
        return `${name}-${seed}-0-D3-${mix}`;
    };
    const copyToClipboard = async (text: string): Promise<void> => {
        try {
            await navigator.clipboard.writeText(text);
        } catch (err) {}
    };
    const onReroll = () => {
        let nseed = traitsToNumber();
        if (nseed === 0) {
            nseed = Math.round(Math.random() * 0x7fffffff);
        }
        generateWorld(nseed);
    };
    const onCopy = () => copyToClipboard(getSeedString(seed));
    const onSubmit = () => {
        setDrawer(false);
        let nseed = traitsToNumber();
        if (nseed === 0) {
            if (seed.length === 0) {
                nseed = Math.round(Math.random() * 0x7fffffff);
            } else {
                nseed = parseInt(seed);
            }
        }
        generateWorld(nseed);
    };
    return (
        <>
            <InputGroup>
                <Button onClick={() => setDrawer(true)}>
                    {translation("Settings")}
                </Button>
                <Form.Control
                    id="seed"
                    type="text"
                    value={seed}
                    placeholder={translation("Worldgen Seed")}
                    onChange={(e) => setSeed(e.target.value)}
                    onKeyDown={(e) => {
                        if (e.key === "Enter") {
                            generateWorld(parseInt(seed));
                        }
                    }}
                />
                <Button key="reroll" onClick={onReroll}>
                    {translation("Reroll")}
                </Button>
                <Button key="copy" onClick={onCopy}>
                    {translation("Copy")}
                </Button>
            </InputGroup>
            <Offcanvas show={drawer} onHide={onSubmit}>
                <Navbar className="justify-content-between">
                    <Container>
                        <div className="hstack">
                            <Button onClick={onSubmit}>
                                {translation("Submit")}
                            </Button>
                        </div>
                        <div className="d-flex d-md-none hstack gap-3">
                            <NavRight onSetAppConfig={onSetAppConfig} />
                        </div>
                    </Container>
                </Navbar>
                <Offcanvas.Body>
                    <Settings
                        cluster={cluster}
                        mixings={mixings}
                        traits={traits}
                        onChange={(cluster, mixings, traits) => {
                            setCluster(cluster);
                            setTraits(traits);
                            setMixings(mixings);
                        }}
                    />
                </Offcanvas.Body>
            </Offcanvas>
        </>
    );
};

export default ToolBar;
