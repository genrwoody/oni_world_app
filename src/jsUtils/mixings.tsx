import React from "react";
import FloatingLabel from "react-bootstrap/FloatingLabel";
import Form from "react-bootstrap/Form";
import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";

import configuration from "./configuration";
import useTranslation from "./language";

const toBase5 = (num: number) => {
    const result: number[] = new Array(configuration.mixing.length).fill(0);
    let index = 0;
    while (num > 0) {
        result[index] = num % 5;
        num = Math.floor(num / 5);
        index += 1;
    }
    return result;
};

const fromBase5 = (nums: Array<number>) => {
    let result = 0;
    for (let i = nums.length; i > 0; i--) {
        result += nums[i - 1] * Math.pow(5, i - 1);
    }
    return result;
};

interface ConfigProps {
    index: number;
    name: string;
    active: number;
    onSetActive: (active: number) => void;
}

const MixingItem = ({ name, active, onSetActive }: ConfigProps) => {
    const translation = useTranslation();
    const threeOptions = ["Disable", "Likely", "Guaranteed"];
    return (
        <Col xs className="mb-3">
            <FloatingLabel label={translation(name)}>
                <Form.Select
                    value={active}
                    onChange={(e) => onSetActive(parseInt(e.target.value))}
                >
                    {threeOptions.map((item, index) => (
                        <option key={index} value={index}>
                            {translation(item)}
                        </option>
                    ))}
                </Form.Select>
            </FloatingLabel>
        </Col>
    );
};

export const Mixings = ({ index, name, active, onSetActive }: ConfigProps) => {
    const translation = useTranslation();

    const pack = configuration.mixing.find((item) => item.key === name)!;
    const options = toBase5(active);
    const show = (key: string) => {
        if (key[2] === "p") {
            return false;
        }
        if (key[2] === "f" && index === 0) {
            return false;
        }
        return key[1] === pack.key[1];
    };
    return (
        <Form.Group className="mb-3">
            <Form.Switch
                id={pack.key}
                className="mb-3"
                label={translation(pack.name)}
                checked={options[pack.type] === 1}
                onChange={(e) => {
                    options[pack.type] = e.target.checked ? 1 : 0;
                    onSetActive(fromBase5(options));
                }}
            />
            <Row xs={2} hidden={pack.key === "d3p1" || !options[pack.type]}>
                {configuration.mixing
                    .filter((item) => show(item.key))
                    .map((item, index) => (
                        <MixingItem
                            key={index}
                            index={index}
                            name={item.name}
                            active={options[item.type]}
                            onSetActive={(active: number) => {
                                options[item.type] = active;
                                onSetActive(fromBase5(options));
                            }}
                        />
                    ))}
            </Row>
        </Form.Group>
    );
};

export default Mixings;
