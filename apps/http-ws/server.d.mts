export function getRequestHandler(serverOpts: any): (req: any, res: any) => boolean;
export function openServer(opts: any, cbAccept: any, cbConnect: any): Server;
declare class Server extends Events {
    constructor(server: any, serverOpts: any, reqHandler: any);
    handlers: any[];
    resourcePath: string;
    npmPath: string;
    app: any;
    reqHandler: any;
    serverOpts: any;
    server: any;
    setResourcePath(path: any): void;
    addHandler(handler: any): void;
    removeHandler(handler: any): void;
    handleEvent(req: any, res: any): boolean;
}
import { Events } from "../events/events.mjs";
export {};
