export function uExpress(): UExpressApp;
export default uExpress;
export type UExpressRequest = {
    [x: string]: unknown;
    url: string | null;
    method?: string;
    path?: string;
    query?: URLSearchParams;
    headers?: {
        [x: string]: string;
    };
    connection?: {
        [x: string]: unknown;
        headers?: {
            [x: string]: string;
        };
    };
};
export type UExpressResponse = {
    [x: string]: unknown;
    headersSent?: boolean;
    writeHead: (statusCode: number, headers?: {
        [x: string]: string;
    }) => void;
    end: (body?: unknown) => void;
};
export type UExpressNext = () => void;
export type UExpressHandler = (req: UExpressRequest, res: UExpressResponse, next: UExpressNext) => unknown | Promise<unknown>;
export type UExpressRoute = string | RegExp;
export type UExpressApp = {
    use: ((handler: UExpressHandler) => void) & ((route: UExpressRoute, handler: UExpressHandler) => void);
    all: (route: UExpressRoute, handler: UExpressHandler) => void;
    get: (route: UExpressRoute, handler: UExpressHandler) => void;
    post: (route: UExpressRoute, handler: UExpressHandler) => void;
    handle: (req: UExpressRequest, res: UExpressResponse) => boolean;
};
