declare module 'libmrp_napi.so' {
  export function init(options: {
    workDir: string;
    width: number;
    height: number;
    debug?: boolean;
  }): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function loadPackage(packagePath: string): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function start(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function pause(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function resume(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function sendInput(event: {
    type: 'key' | 'touch';
    action: 'down' | 'up' | 'move';
    keyCode?: number;
    x?: number;
    y?: number;
    timestamp?: number;
  }): { ok: boolean; errorCode?: number; errorMessage?: string };

  export function pullFrame(): {
    ok: boolean;
    hasNewFrame: boolean;
    width: number;
    height: number;
    pixelFormat: 'RGBA_8888';
    buffer?: ArrayBuffer;
    frameId?: number;
    errorCode?: number;
    errorMessage?: string;
  };

  export function release(): { ok: boolean; errorCode?: number; errorMessage?: string };
}
