import React from "react";
import { useState, useEffect, useContext } from "react";
import { createRoot } from "react-dom/client";
import Navbar from "react-bootstrap/Navbar";
import Container from "react-bootstrap/Container";
import Stack from "react-bootstrap/Stack";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import Card from "react-bootstrap/Card";
import Modal from "react-bootstrap/Modal";
import Tabs from "react-bootstrap/Tabs";
import Tab from "react-bootstrap/Tab";

import { LanguageContext, useTranslation } from "./jsUtils/language";
import configuration from "./jsUtils/configuration";
import WorldCanvas from "./jsUtils/worldcanvas";
import ToolBar from "./jsUtils/toolbar";
import NavRight from "./jsUtils/navright";
import { updateWorld, ThemeContext } from "./jsUtils";

import "bootstrap/dist/css/bootstrap.min.css";
import "./jsUtils/index.css";
import zonesImageUrl from "../asset/zones.png";

interface WorldInfoProps {
    world: World;
    onSetFocus: (index: number) => void;
}

const WorldInfo = ({ world, onSetFocus }: WorldInfoProps) => {
    const translation = useTranslation();
    const convert = (color?: number) => {
        const list = ["success", "danger", "primary"];
        return color ? list[color - 1] : undefined;
    };
    const traits = configuration.traits;
    return (
        <>
            <Row xs={2} md={4}>
                {world.traits.map((item, index) => (
                    <Card key={index} text={convert(traits[item].type)}>
                        <Card.Body style={{ padding: "0.75rem 0" }}>
                            {translation(traits[item].name)}
                        </Card.Body>
                    </Card>
                ))}
            </Row>
            <Row xs={2} md={4}>
                {world.geysers.map((item, index) => (
                    <Card
                        key={index}
                        text={convert(item.desc.type)}
                        onMouseEnter={() => onSetFocus(index)}
                        onMouseLeave={() => onSetFocus(-1)}
                    >
                        <Card.Body style={{ padding: "0.75rem 0" }}>
                            {translation(item.desc.name)}
                        </Card.Body>
                    </Card>
                ))}
            </Row>
        </>
    );
};

const Biomes = () => {
    const biomes = [
        "Tundra Biome",
        "",
        "Marsh Biome",
        "Sandstone Biome",
        "Jungle Biome",
        "Magma Biome",
        "Oily Biome",
        "Space Biome",
        "Ocean Biome",
        "Rust Biome",
        "Forest Biome",
        "Radioactive Biome",
        "Swampy Biome",
        "Wasteland Biome",
        "",
        "Metallic Biome",
        "Barren Biome",
        "Moo Biome",
        "Ice Cave Biome",
        "Cool Pool Biome",
        "Nectar Biome",
        "Garden Biome",
        "Feather Biome",
        "Wetlands Biome",
        "Kelp Forest Biome",
        "Reef Biome",
        "Abyss Biome",
        "Beach Biome",
    ];
    const translation = useTranslation();
    return (
        <Row xs={3}>
            {biomes.map(
                (item, index) =>
                    item !== "" && (
                        <Card key={index}>
                            <Card.Body style={{ padding: "0.75rem 0" }}>
                                <span
                                    className={"biome-icon icon" + index}
                                ></span>
                                <span>{translation(item)}</span>
                            </Card.Body>
                        </Card>
                    ),
            )}
        </Row>
    );
};

const createSprite = (e: Event) => {
    const promises = [];
    const image = e.target as HTMLImageElement;
    for (let i = 0; i < 6; i++) {
        for (let j = 0; j < 5; j++) {
            const promise = createImageBitmap(image, j * 32, i * 32, 32, 32);
            promises.push(promise);
        }
    }
    Promise.all(promises).then((sprites) => Module.sprite.push(...sprites));
};

interface AppProps {
    onSetLanguage: (lang: string) => void;
    onSetTheme: (theme: number) => void;
}

const App = ({ onSetLanguage, onSetTheme }: AppProps) => {
    const [loading, setLoading] = useState(true);
    const [worlds, setWorlds] = useState(new Array<World>());
    const [focus, setFocus] = useState(-1);
    const language = useContext(LanguageContext);
    const translation = useTranslation();
    useEffect(() => {
        document.title = translation("ONI World Generator");
    }, [language]);
    useEffect(() => {
        if (Module.wasm !== undefined) return;
        Module.wasm = null;
        Module.worlds = [];
        Module.sprite = [];
        Module.updateWorld = updateWorld;
        Module.onRuntimeInitialized = () => {
            Module.app_init(new Date().getTime() & 0x7fffffff);
            setLoading(false);
        };
        setLoading(true);
        const load = async (url: string) => {
            const response = await fetch(url, { credentials: "same-origin" });
            return response.arrayBuffer();
        };
        Promise.all([
            load("./data.bin"),
            load("./wasm.bin"),
        ])
            .then((buffers) => {
                Module.data = new Uint8Array(buffers[0], 40);
                Module.wasm = new Uint8Array(buffers[1]);
                const script = document.createElement("script");
                script.src = "./oniWorldApp.js";
                script.async = true;
                document.body.appendChild(script);
            })
            .catch((reason) => console.log("fetch error: " + reason));
        const image = new Image();
        image.onload = createSprite;
        image.src = zonesImageUrl;
        //if ("serviceWorker" in navigator) {
        //    navigator.serviceWorker.register("./serviceworker.js");
        //}
    }, []);
    const onSetWorlds = () => {
        setFocus(-1);
        if (Module.worlds.length === 0) {
            return;
        }
        if (Module.worlds[0].type !== 0) {
            setWorlds([Module.worlds[1], Module.worlds[0]]);
        } else {
            setWorlds([...Module.worlds]);
        }
    };
    const onSetAppConfig = (lang: string, theme: number) => {
        onSetLanguage(lang);
        onSetTheme(theme);
    };
    const onSetFocus = (world: number, geyser: number) => {
        if (geyser === -1) {
            setFocus(-1);
            return;
        }
        let offset = 0;
        for (let i = 0; i < world; i++) {
            offset += worlds[i].geysers.length;
        }
        setFocus(geyser + offset);
    };
    const tabsElement = () => {
        return (
            <Tabs defaultActiveKey="info" className="mb-3">
                <Tab eventKey="info" title={translation("Information")}>
                    {worlds.map((world, index) => (
                        <WorldInfo
                            key={index}
                            world={world}
                            onSetFocus={(geyser) => onSetFocus(index, geyser)}
                        />
                    ))}
                </Tab>
                <Tab eventKey="biome" title={translation("Biomes")}>
                    <Biomes />
                </Tab>
            </Tabs>
        );
    };
    return (
        <>
            <Navbar className="bg-body-tertiary justify-content-between">
                <Container>
                    <Stack direction="horizontal">
                        <ToolBar
                            onSetAppConfig={onSetAppConfig}
                            onSetWorld={onSetWorlds}
                        />
                    </Stack>
                    <Stack
                        direction="horizontal"
                        className="d-none d-md-flex"
                        gap={3}
                    >
                        <NavRight onSetAppConfig={onSetAppConfig} />
                    </Stack>
                </Container>
            </Navbar>
            <Container>
                <Row>
                    <Col lg={12} xl={6}>
                        {tabsElement()}
                    </Col>
                    <WorldCanvas worlds={worlds} focus={focus} />
                </Row>
            </Container>
            <Modal
                id="loading"
                show={loading}
                backdrop="static"
                keyboard={false}
                centered
            >
                <Modal.Body>
                    {translation("Initializing, please wait a moment.")}
                </Modal.Body>
            </Modal>
        </>
    );
};

const Main: React.FC = () => {
    const initTheme = (): number => {
        return matchMedia("(prefers-color-scheme: dark)").matches ? 1 : 0;
    };
    const [language, setLanguage] = useState(navigator.language);
    const [theme, setTheme] = useState(initTheme());
    useEffect(() => {
        const expect = theme === 0 ? "light" : "dark";
        document.documentElement.setAttribute("data-bs-theme", expect);
    }, [theme]);
    const onSetTheme = (theme: number) => {
        const expect = theme === 0 ? "light" : "dark";
        document.documentElement.setAttribute("data-bs-theme", expect);
        setTheme(theme);
    };
    return (
        <ThemeContext.Provider value={theme}>
            <LanguageContext.Provider value={language}>
                <App onSetLanguage={setLanguage} onSetTheme={onSetTheme} />
            </LanguageContext.Provider>
        </ThemeContext.Provider>
    );
};

const root = createRoot(document.getElementById("root")!);
if (process.env.NODE_ENV === "development") {
    root.render(
        <React.StrictMode>
            <Main />
        </React.StrictMode>,
    );
} else {
    root.render(<Main />);
}
