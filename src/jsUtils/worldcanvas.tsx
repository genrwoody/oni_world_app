import React from "react";
import { useContext, useEffect } from "react";
import Card from "react-bootstrap/Card";
import Col from "react-bootstrap/Col";
import { LanguageContext, useTranslation } from "./language";
import { ThemeContext } from "./index";

interface WorldCanvasProps {
    worlds: Array<World>;
    focus: number;
}

const zonePattern: Array<CanvasPattern> = [];

export const WorldCanvas = ({ worlds, focus }: WorldCanvasProps) => {
    const theme = useContext(ThemeContext);
    const language = useContext(LanguageContext);
    const translation = useTranslation();
    useEffect(() => {
        if (worlds.length === 0) return;
        const cvs = document.getElementById("world") as HTMLCanvasElement;
        cvs.style.aspectRatio = `${cvs.clientWidth} / ${cvs.clientHeight}`;
        cvs.width = cvs.clientWidth;
        cvs.height = cvs.clientHeight;
        let width = 0;
        let height = 0;
        let sumY = 0;
        worlds.forEach((world) => (sumY += world.size.y));
        if (cvs.clientWidth / cvs.clientHeight < worlds[0].size.x / sumY) {
            width = cvs.clientWidth;
            height = (sumY / worlds[0].size.x) * width;
        } else {
            height = cvs.clientHeight;
            width = (worlds[0].size.x / sumY) * height;
        }
        const scale = width / worlds[0].size.x;
        const ctx = cvs.getContext("2d")!;
        ctx.translate((cvs.clientWidth - width) / 2, 0);
        ctx.fillStyle = theme ? "#212529" : "white";
        ctx.fillRect(0, 0, cvs.width, cvs.height);
        ctx.strokeRect(0, 0, width, height);
        if (zonePattern.length === 0) {
            Module.sprite.forEach((sprite) => {
                zonePattern.push(ctx.createPattern(sprite, "repeat")!);
            });
        }
        let offset = 0;
        worlds.forEach((world) => {
            world.sites.forEach((item) => {
                ctx.beginPath();
                item.poly.forEach((point, index) => {
                    if (index === 0) {
                        ctx.moveTo(point.x * scale, point.y * scale + offset);
                    } else {
                        ctx.lineTo(point.x * scale, point.y * scale + offset);
                    }
                });
                ctx.closePath();
                const pattern = zonePattern[item.zone];
                ctx.fillStyle = pattern;
                ctx.fill();
            });
            offset += world.size.y * scale;
        });
        ctx.strokeStyle = "black";
        ctx.fillStyle = "black";
        ctx.font = "16px sans-serif";
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        const startBaseName = ["Printing Pod", "Secondary Asteroid Base", ""];
        let count = 0;
        offset = 0;
        worlds.forEach((world) => {
            ctx.fillStyle = "black";
            ctx.lineWidth = 1;
            world.geysers.forEach((item) => {
                const w = 5 * scale;
                const h = 5 * scale;
                const x = item.pos.x * scale;
                const y = item.pos.y * scale + offset;
                ctx.strokeRect(x - w / 2, y - h, w, h);
            });
            let point = world.starting;
            let text = translation(startBaseName.at(world.type)!);
            ctx.fillStyle = "white";
            ctx.lineWidth = 3;
            ctx.strokeText(text, point.x * scale, point.y * scale + offset);
            ctx.fillText(text, point.x * scale, point.y * scale + offset);
            world.geysers.forEach((item) => {
                const x = item.pos.x * scale;
                const y = item.pos.y * scale + offset;
                ctx.strokeText(translation(item.desc.name), x, y);
                ctx.fillText(translation(item.desc.name), x, y);
            });
            if (count <= focus && focus < count + world.geysers.length) {
                const item = world.geysers[focus - count];
                const x = item.pos.x * scale;
                const y = item.pos.y * scale + offset;
                ctx.fillStyle = "red";
                ctx.strokeText(translation(item.desc.name), x, y);
                ctx.fillText(translation(item.desc.name), x, y);
            }
            ctx.lineWidth = 1;
            count += world.geysers.length;
            if (world.type === 0) {
                ctx.fillStyle = "lightgray";
                ctx.fillText(world.coord, (world.size.x / 2) * scale, 15);
            }
            offset += world.size.y * scale;
        });
    }, [worlds, focus, language, theme]);
    return (
        <Col>
            <Card className="world-canvas-container">
                <canvas id="world" className="world-canvas"></canvas>
            </Card>
        </Col>
    );
};

export default WorldCanvas;
