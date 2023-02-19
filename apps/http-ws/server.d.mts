export function getRequestHandler(serverOpts: any): (req: any, res: any) => boolean;
export function openServer(opts: any, cbAccept: any, cbConnect: any): {
    setResourcePath(path: any): void;
    addHandler(handler: any): void;
    removeHandler(handler: any): void;
};
