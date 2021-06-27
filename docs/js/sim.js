
'use strict';

var gl = null;
var params = null;
var shaders = null;
var fields = null;

var sim = {
    canvas: null,
    context: null,
    quad: null,
    borders: null,
    currentTime: 0,
    delta_t: 0,
    paused: false,
    fpsDisplay: null
}

var env = {
    isMobile: false,
    isFirefox: false,
    textureType: null,
    supportsLinearSampling: false,
    filtering: null
}

var impulse = {
    forceActive: false,
    inkActive: false,
    currentPos: null,
    prevPos: null,
    delta: null,
    radial: false
}

window.onload = init;
window.addEventListener('error', (e) => { alert("Error: " + e.message) });

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FBO {
    constructor(width, height) {
        this.texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, env.filtering);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, env.filtering);
        // HACK: find a better way to handle firefox
        let format = env.isFirefox ? gl.RGBA : gl.RGB;
        gl.texImage2D(gl.TEXTURE_2D, 0, format, width, height, 0, format, env.textureType, null);

        this.buffer = gl.createFramebuffer();
        gl.bindFramebuffer(gl.FRAMEBUFFER, this.buffer);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, this.texture, 0);

        let status = gl.checkFramebufferStatus(gl.FRAMEBUFFER);
        if (status != gl.FRAMEBUFFER_COMPLETE) {
            throw `Could not create FBO, status: ${status}`;
        }

        gl.clearColor(0, 0, 0, 0);
        gl.clear(gl.COLOR_BUFFER_BIT);

        gl.bindTexture(gl.TEXTURE_2D, null);
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    }

    clear() {
        gl.bindFramebuffer(gl.FRAMEBUFFER, this.buffer);
        gl.clearColor(0, 0, 0, 0);
        gl.clear(gl.COLOR_BUFFER_BIT);
    }
}

class SwapFBO {
    constructor(width, height) {
        this.front = new FBO(width, height);
        this.back = new FBO(width, height);
    }

    swap() {
        let temp = this.front;
        this.front = this.back;
        this.back = temp;
    }

    clear() {
        this.front.clear();
        this.back.clear();
    }
}

class VertexList {
    constructor(vertices, indices) {
        this.vbo = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vbo);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);

        this.ebo = gl.createBuffer();
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.ebo);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);
        gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);

        gl.bindBuffer(gl.ARRAY_BUFFER, null);
    }

    bind() {
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vbo);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.ebo);
    }
}

class ShaderProgram {
    constructor(vert, frag) {
        this.vert = vert;
        this.frag = frag;
        this.program = gl.createProgram();
        gl.attachShader(this.program, this.vert.shader);
        gl.attachShader(this.program, this.frag.shader);
        gl.linkProgram(this.program);

        if (!gl.getProgramParameter(this.program, gl.LINK_STATUS)) {
            let err = gl.getProgramInfoLog(this.program);
            throw `An error occured while linking a shader program: ${err}`;
        }
    }

    use() {
        gl.useProgram(this.program);
    }

    getUniformLocation(name) {
        let loc = gl.getUniformLocation(this.program, name);
        if (loc == null) {
            //let count = gl.getProgramParameter(this.program, gl.ACTIVE_UNIFORMS);
            //for (let i = 0; i < count; i++) {
            //    let uname = gl.getActiveUniform(this.program, i).name;
            //    console.log('Uniform: ' + uname);
            //}

            //throw `Uniform not found: ${name}`;
        }
        return loc;
    }

    setFloat(name, value) {
        gl.uniform1f(this.getUniformLocation(name), value);
    }

    setInt(name, value) {
        gl.uniform1i(this.getUniformLocation(name), value);
    }

    setTexture(name, texture, unit) {
        this.setInt(name, unit);
        gl.activeTexture(gl.TEXTURE0 + unit);
        gl.bindTexture(gl.TEXTURE_2D, texture);
    }

    setVec2(name, x, y) {
        gl.uniform2f(this.getUniformLocation(name), x, y);
    }

    setVec2(name, vec) {
        gl.uniform2f(this.getUniformLocation(name), vec.x, vec.y);
    }

    setVec3(name, x, y, z) {
        gl.uniform3f(this.getUniformLocation(name), x, y, z);
    }

    setVec3(name, vec) {
        gl.uniform3f(this.getUniformLocation(name), vec.x, vec.y, vec.z);
    }

    setVec4(name, x, y, z, w) {
        gl.uniform4f(this.getUniformLocation(name), x, y, z, w);
    }
}

class Shader {
    constructor(type, source) {
        this.shader = gl.createShader(type);
        gl.shaderSource(this.shader, source);
        gl.compileShader(this.shader);

        if (!gl.getShaderParameter(this.shader, gl.COMPILE_STATUS)) {
            let error = gl.getShaderInfoLog(this.shader);
            gl.deleteShader(this.shader);
            throw `An error occurred compiling a shader: ${error}`;
        }
    }
}

class Vec2 {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }

    subtract(other) {
        return new Vec2(this.x - other.x, this.y - other.y);
    }

    mul(vec) {
        return new Vec2(this.x * vec.x, this.y * vec.y);
    }

    mulf(scale) {
        return new Vec2(this.x * scale, this.y * scale);
    }
}

class Vec3 {
    constructor(x, y, z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    subtract(other) {
        return new Vec3(this.x - other.x, this.y - other.y, this.z - other.z);
    }
}

function init() {
    env.isMobile = /Android|webOS|iPhone|iPad|Mac|Macintosh|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
    env.isFirefox = /Firefox/i.test(navigator.userAgent);

    sim.canvas = document.getElementById('mainCanvas');
    
    if (env.isMobile) {
        let w = (window.innerWidth > 0) ? window.innerWidth : screen.width;
        if (w <= 400) {
            sim.canvas.width = w;
            sim.canvas.height = w;
        }
    }

    gl = sim.canvas.getContext('webgl');

    if (gl == null) {
        alert('Your browser or device does not supported webgl :(');
        return;
    }

    // On my iphone and firefox this extension is supported but trying to render to it doesn't work
    if (!env.isMobile && !env.isFirefox) {
        let fullFloatExt = gl.getExtension('OES_texture_float');
        if (fullFloatExt != null) {
            env.textureType = gl.FLOAT;

            let linearFilteringExt = gl.getExtension('OES_texture_float_linear');
            if (linearFilteringExt != null) {
                env.supportsLinearSampling = true;
            }
        }
    }

    if (env.textureType == null) {
        let halfFloatExt = gl.getExtension('OES_texture_half_float');
        if (halfFloatExt != null) {
            env.textureType = halfFloatExt.HALF_FLOAT_OES;

            let linearFilteringExt = gl.getExtension('OES_texture_half_float_linear');
            if (linearFilteringExt != null) {
                env.supportsLinearSampling = true;
            }
        }
    }

    if (env.textureType == null) {
        alert('Your browser or device does not support floating point textures :(');
        return;
    }

    env.filtering = env.supportsLinearSampling ? gl.LINEAR : gl.NEAREST;

    sim.fpsDisplay = document.getElementById('fpsText');

    let w = gl.canvas.width;
    let h = gl.canvas.height;

    let c = new Vec2(1 - 1.5/w, 1 - 1.5/h);
    let quadVerts = [c.x, -c.y,
                     c.x,  c.y,
                    -c.x,  c.y,
                    -c.x, -c.y]

    sim.quad = new VertexList(quadVerts, [0, 1, 3, 1, 2, 3]);

    c = new Vec2(1 - 0.5/w, 1 - 0.5/h);
    let borderVerts = [c.x, -c.y,
                       c.x,  c.y,
                      -c.x,  c.y,
                      -c.x, -c.y];

    sim.borders = {
        top: new VertexList(borderVerts, [3, 0]),
        bottom: new VertexList(borderVerts, [1, 2]),
        left: new VertexList(borderVerts, [2, 3]),
        right: new VertexList(borderVerts, [0, 1])
    }

    let vshader = new Shader(gl.VERTEX_SHADER, glsl.vert);

    shaders = {
        advection: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.adv)),
        impulse: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.impulse)),
        vorticity: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.vorticity)),
        addVorticity: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.addVorticity)),
        jacobi: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.jacobi)),
        gradient: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.gradient)),
        divergence: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.divergence)),
        subtract: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.subtract)),
        boundaries: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.boundary)),

        scalarVis: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.scalar_vis)),
        vectorVis: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.vector_vis)),

        copy: new ShaderProgram(vshader, new Shader(gl.FRAGMENT_SHADER, glsl.copy))
    }

    let rdv = new Vec2(1 / w, 1 / h);
    for (const prop in shaders) {
        shaders[prop].use();
        shaders[prop].setVec2('stride', rdv);
    }

    fields = {
        velocity: new SwapFBO(w, h),
        velocityVis: new FBO(w, h),
        ink: new SwapFBO(w, h),
        inkVis: new FBO(w, h),
        pressure: new SwapFBO(w, h),
        pressureVis: new FBO(w, h),
        vorticity: new FBO(w, h),
        vorticityVis: new FBO(w, h),
        temp: new FBO(w, h)
    }

    params = {
        selfAdvect: true,
        advectInk: true,
        diffuseVelocity: true,
        diffuseInk: true,
        addVorticity: true,
        computeBoundaries: false, // TODO: fix boundary enforcement

        rdv: rdv,
        gridScale: 0.3,
        viscosity: 0.001,
        inkViscosity: 0.00001,
        vorticity: 0.01,
        forceRadius: 0.002,
        advectionDissipation: 0.99,
        inkAdvectionDissipation: 0.98,
        inkVolume: 0.002,

        inkColour: new Vec3(0.54, 0.2, 0.78),
        rainbowModeHue: null,
        rainbowModeEnabled: false,

        displayField: 'ink'
    }

    setupParamsForm(params);

    sim.canvas.addEventListener('mousedown', e => mouseEvent('down', e, false));
    sim.canvas.addEventListener('mouseup', e => mouseEvent('up', e, false));
    sim.canvas.addEventListener('mousemove', e => mouseEvent('move', e, false));

    sim.canvas.addEventListener('touchstart', e => mouseEvent('down', e, true));
    sim.canvas.addEventListener('touchend', e => mouseEvent('up', e, true));
    sim.canvas.addEventListener('touchmove', e => mouseEvent('move', e, true));

    document.getElementById("rdInk").addEventListener('change', updateDisplayField);
    document.getElementById("rdVelocity").addEventListener('change', updateDisplayField);
    document.getElementById("rdPressure").addEventListener('change', updateDisplayField);
    document.getElementById("rdVorticity").addEventListener('change', updateDisplayField);

    document.getElementById('btnUpdate').addEventListener('click', updateFromParamVars);
    document.getElementById('btnClear').addEventListener('click', clearFields);
    document.getElementById('btnPause').addEventListener('click', toggleSimulation);
    document.addEventListener('keydown', e => { if (e.key == 'p') { toggleSimulation(); } });

    document.getElementById('clrInk').value = rgbfToHex(params.inkColour);
    document.getElementById('clrInk').addEventListener('change', (e) => {
        params.inkColour = hexToRgb(e.target.value);
    });

    window.requestAnimationFrame(tick);
}

function getRelativeMousePos(mouse) {
    let rect = sim.canvas.getBoundingClientRect();
    let scx = sim.canvas.width / rect.width;
    let scy = sim.canvas.height / rect.height;
    let x = (mouse.clientX - rect.left) * scx;
    let y = (mouse.clientY - rect.top) * scy;
    return new Vec2(x, y);
}

function mouseEvent(type, event, isTouch) {
    let pos;

    if (isTouch) {
        event.preventDefault();
        let idx = event.changedTouches.length - 1;
        pos = getRelativeMousePos(event.changedTouches[idx]);
    }
    else {
        pos = getRelativeMousePos(event);
    }

    if (type == 'down') {
        if (!impulse.forceActive && !impulse.inkActive) {
            impulse.forceActive = true;
            impulse.inkActive = true;
            impulse.currentPos = new Vec2(pos.x, sim.canvas.height - pos.y);
            impulse.lastPos = impulse.currentPos;
            impulse.delta = new Vec2(0, 0);
        }
    }
    else if (type == 'up') {
        impulse.currentPos = null;
        impulse.lastPos = null;
        impulse.forceActive = false;
        impulse.inkActive = false;
    }
    else if (type == 'move') {
        if (impulse.forceActive || impulse.inkActive) {
            let temp = impulse.currentPos;
            impulse.currentPos = new Vec2(pos.x, sim.canvas.height - pos.y);
            impulse.lastPos = temp;
            impulse.delta = impulse.currentPos.subtract(impulse.lastPos);
        }
    }
}

function setupParamsForm(params) {
    document.getElementById("rdInk").checked = params.displayField == 'ink';
    document.getElementById("rdVelocity").checked = params.displayField == 'velocity';
    document.getElementById("rdPressure").checked = params.displayField == 'pressure';
    document.getElementById("rdVorticity").checked = params.displayField == 'vorticity';

    document.getElementById("chkSelfAdvect").checked = params.selfAdvect;
    document.getElementById("chkSelfAdvect").addEventListener('change', updateParamFlags);

    document.getElementById("chkInkAdvect").checked = params.advectInk;
    document.getElementById("chkInkAdvect").addEventListener('change', updateParamFlags);

    document.getElementById("chkDiffuse").checked = params.diffuseVelocity;
    document.getElementById("chkDiffuse").addEventListener('change', updateParamFlags);

    document.getElementById("chkInkDiffuse").checked = params.diffuseInk;
    document.getElementById("chkInkDiffuse").addEventListener('change', updateParamFlags);

    document.getElementById("chkVorticity").checked = params.addVorticity;
    document.getElementById("chkVorticity").addEventListener('change', updateParamFlags);

    document.getElementById("chkBoundaries").checked = params.computeBoundaries;
    document.getElementById("chkBoundaries").addEventListener('change', updateParamFlags);

    document.getElementById("chkRainbow").checked = params.rainbowModeEnabled;
    document.getElementById("chkRainbow").addEventListener('change', toggleRainbowMode);

    document.getElementById('txtGridScale').value = params.gridScale;
    document.getElementById('txtVorticity').value = params.vorticity;
    document.getElementById('txtViscosity').value = params.viscosity;
    document.getElementById('txtInkViscosity').value = params.inkViscosity;
    document.getElementById('txtDissipation').value = params.advectionDissipation;
    document.getElementById('txtInkDissipation').value = params.inkAdvectionDissipation;
    document.getElementById('txtRadius').value = params.forceRadius;
    document.getElementById('txtVolume').value = params.inkVolume;
}

function updateParamFlags() {
    params.selfAdvect = document.getElementById("chkSelfAdvect").checked;
    params.advectInk = document.getElementById("chkInkAdvect").checked;
    params.diffuseVelocity = document.getElementById("chkDiffuse").checked;
    params.diffuseInk = document.getElementById("chkInkDiffuse").checked;
    params.addVorticity = document.getElementById("chkVorticity").checked;
    params.computeBoundaries = document.getElementById("chkBoundaries").checked;
}

function updateFromParamVars() {
    params.gridScale = parseFloat(document.getElementById('txtGridScale').value);
    params.vorticity = parseFloat(document.getElementById('txtVorticity').value);
    params.viscosity = parseFloat(document.getElementById('txtViscosity').value);
    params.inkViscosity = parseFloat(document.getElementById('txtInkViscosity').value);
    params.advectionDissipation = parseFloat(document.getElementById('txtDissipation').value);
    params.inkAdvectionDissipation = parseFloat(document.getElementById('txtInkDissipation').value);
    params.forceRadius = parseFloat(document.getElementById('txtRadius').value);
    params.inkVolume = parseFloat(document.getElementById('txtVolume').value);
}

function updateDisplayField() {
    if (params.displayField = document.getElementById("rdInk").checked) {
        params.displayField = 'ink';
    }
    else if (params.displayField = document.getElementById("rdVelocity").checked) {
        params.displayField = 'velocity';
    }
    else if (params.displayField = document.getElementById("rdPressure").checked) {
        params.displayField = 'pressure';
    }
    else if (params.displayField = document.getElementById("rdVorticity").checked) {
        params.displayField = 'vorticity';
    }
}

function toggleSimulation() {
    let btn = document.getElementById('btnPause');
    sim.paused = !sim.paused;
    if (sim.paused) {
        btn.innerText = 'Play';
    }
    else {
        btn.innerText = 'Pause';
    }
}

function toggleRainbowMode() {
    params.rainbowModeEnabled = !params.rainbowModeEnabled;

    if (params.rainbowModeEnabled) {
        params.rainbowModeHue = 0;
    }
}

function clearFields() {
    for (const field in fields) {
        fields[field].clear();
    }
}

function tick(timestamp) {
    timestamp /= 1000;
    sim.delta_t = sim.currentTime == 0 ? 0.016667 : timestamp - sim.currentTime;
    sim.currentTime = timestamp;

    if (!sim.paused) {
        let fps = (1 / sim.delta_t).toFixed(2);
        sim.fpsDisplay.innerText = `FPS: ${fps} Hz`;

        computeFields();
    }

    let outputTexture = null;

    if (params.displayField == 'ink') {
        outputTexture = fields.ink.front.texture;
    }
    else if (params.displayField == 'velocity') {
        shaders.vectorVis.use();
        shaders.vectorVis.setVec4('bias', 0.5, 0.5, 0.5, 0.5);
        shaders.vectorVis.setVec4('scale', 0.5, 0.5, 0.5, 0.5);
        shaders.vectorVis.setTexture('field', fields.velocity.front.texture, 0);
        drawQuad(fields.velocityVis.buffer);

        outputTexture = fields.velocityVis.texture;
    }
    else if (params.displayField == 'pressure') {
        shaders.scalarVis.use();
        shaders.scalarVis.setVec4('bias', 0, 0, 0, 0);
        shaders.scalarVis.setVec4('scale', 2, -1, -2, 1);
        shaders.scalarVis.setTexture('field', fields.pressure.front.texture, 0);
        drawQuad(fields.pressureVis.buffer);

        outputTexture = fields.pressureVis.texture;
    }
    else {
        shaders.scalarVis.use();
        shaders.scalarVis.setVec4('bias', 0, 0, 0, 0);
        shaders.scalarVis.setVec4('scale', 1, 1, -1, -1);
        shaders.scalarVis.setTexture('field', fields.vorticity.texture, 0);
        drawQuad(fields.vorticityVis.buffer);

        outputTexture = fields.vorticityVis.texture;
    }

    shaders.copy.use();
    shaders.copy.setTexture('field', outputTexture, 0);
    drawQuad(null);

    window.requestAnimationFrame(tick);
}

function computeFields() {
    if (params.selfAdvect) {
        computeBoundaries(fields.velocity, -1);

        shaders.advection.use();
        shaders.advection.setFloat('delta_t', sim.delta_t);
        shaders.advection.setFloat('gs', params.gridScale);
        shaders.advection.setFloat('dissipation', params.advectionDissipation);
        shaders.advection.setTexture('velocity', fields.velocity.front.texture, 0);
        shaders.advection.setTexture('quantity', fields.velocity.front.texture, 1);

        drawQuad(fields.velocity.back.buffer);
        fields.velocity.swap();
    }

    if (params.advectInk) {
        computeBoundaries(fields.ink, 0);

        shaders.advection.use();
        shaders.advection.setFloat('delta_t', sim.delta_t);
        shaders.advection.setFloat('gs', params.gridScale);
        shaders.advection.setFloat('dissipation', params.inkAdvectionDissipation);
        shaders.advection.setTexture('velocity', fields.velocity.front.texture, 0);
        shaders.advection.setTexture('quantity', fields.ink.front.texture, 1);

        drawQuad(fields.ink.back.buffer);
        fields.ink.swap();
    }

    if (impulse.forceActive) {
        let diff = impulse.delta;

        let force = new Vec3(Math.min(Math.max(diff.x, -1 * params.gridScale), params.gridScale),
            Math.min(Math.max(diff.y, -1 * params.gridScale), params.gridScale),
            0);

        shaders.impulse.use();
        shaders.impulse.setFloat('radius', params.forceRadius);
        shaders.impulse.setVec2('position', impulse.currentPos.mul(params.rdv));
        shaders.impulse.setVec3('force', force);
        shaders.impulse.setTexture('velocity', fields.velocity.front.texture, 0);
        drawQuad(fields.velocity.back.buffer);
        fields.velocity.swap();
    }

    if (impulse.inkActive) {
        let colour;
        if (params.rainbowModeEnabled) {

            params.rainbowModeHue += sim.delta_t / 0.016667;
            if (params.rainbowModeHue > 360) {
                params.rainbowModeHue = 0;
            }

            colour = hslToRgb(params.rainbowModeHue / 360, 1, 0.5);
        }
        else {
            colour = params.inkColour;
        }

        shaders.impulse.use();
        shaders.impulse.setFloat('radius', params.inkVolume);
        shaders.impulse.setVec2('position', impulse.currentPos.mul(params.rdv));
        shaders.impulse.setVec3('force', colour);
        shaders.impulse.setTexture('velocity', fields.ink.front.texture, 0);
        drawQuad(fields.ink.back.buffer);
        fields.ink.swap();
    }

    if (params.addVorticity) {
        shaders.vorticity.use();
        shaders.vorticity.setFloat('gs', params.gridScale);
        shaders.vorticity.setTexture('velocity', fields.velocity.front.texture, 0);
        drawQuad(fields.vorticity.buffer);

        computeBoundaries(fields.velocity, -1);

        shaders.addVorticity.use();
        shaders.addVorticity.setFloat('gs', params.gridScale);
        shaders.addVorticity.setFloat('delta_t', sim.delta_t);
        shaders.addVorticity.setFloat('scale', params.vorticity);
        shaders.addVorticity.setTexture('velocity', fields.velocity.front.texture, 0);
        shaders.addVorticity.setTexture('vorticity', fields.vorticity.texture, 1);
        drawQuad(fields.velocity.back.buffer);
        fields.velocity.swap();
    }

    if (params.diffuseVelocity) {
        let alpha = (params.gridScale * params.gridScale) / (params.viscosity * sim.delta_t);
        let beta = alpha + 4;
        solvePoissonSystem(fields.velocity, fields.velocity.front, alpha, beta);
    }

    if (params.diffuseInk) {
        let alpha = (params.gridScale * params.gridScale) / (params.inkViscosity * sim.delta_t);
        let beta = alpha + 4;
        solvePoissonSystem(fields.ink, fields.ink.front, alpha, beta);
    }

    // Calculate div(W)
    shaders.divergence.use();
    shaders.divergence.setFloat('gs', params.gridScale);
    shaders.divergence.setTexture('field', fields.velocity.front.texture, 0);
    drawQuad(fields.velocity.back.buffer);

    // Solve for P in: Laplacian(P) = div(W)
    solvePoissonSystem(fields.pressure, fields.velocity.back, -params.gridScale * params.gridScale, 4);

    // Calculate grad(P)
    shaders.gradient.use();
    shaders.gradient.setFloat('gs', params.gridScale);
    shaders.gradient.setTexture('field', fields.pressure.front.texture, 0);
    drawQuad(fields.pressure.back.buffer);

    // Calculate U = W - grad(P) where div(U)=0
    shaders.subtract.use();
    shaders.subtract.setTexture('a', fields.velocity.front.texture, 0);
    shaders.subtract.setTexture('b', fields.pressure.back.texture, 1);
    drawQuad(fields.velocity.back.buffer);
    fields.velocity.swap();
}

function computeBoundaries(swapFBO, scale) {
    if (!params.computeBoundaries) {
        return;
    }

    copyFBO(swapFBO.back, swapFBO.front);
    shaders.boundaries.use();
    shaders.boundaries.setFloat('scale', scale);
    shaders.boundaries.setVec2('rdv', params.rdv);
    shaders.boundaries.setTexture('field', swapFBO.front.texture, 0);

    gl.enableVertexAttribArray(0);
    gl.bindFramebuffer(gl.FRAMEBUFFER, swapFBO.back.buffer);

    shaders.boundaries.setVec2('offset', 0, -1);
    sim.borders.top.bind();
    gl.drawElements(gl.LINES, 2, gl.UNSIGNED_SHORT, 0);

    shaders.boundaries.setVec2('offset', 0, 1);
    sim.borders.bottom.bind();
    gl.drawElements(gl.LINES, 2, gl.UNSIGNED_SHORT, 0);

    shaders.boundaries.setVec2('offset', 1, 0);
    sim.borders.left.bind();
    gl.drawElements(gl.LINES, 2, gl.UNSIGNED_SHORT, 0);

    shaders.boundaries.setVec2('offset', -1, 0);
    sim.borders.right.bind();
    gl.drawElements(gl.LINES, 2, gl.UNSIGNED_SHORT, 0);

    swapFBO.swap();
}

function solvePoissonSystem(swapFBO, initialValue, alpha, beta) {
    copyFBO(fields.temp, initialValue);
    shaders.jacobi.use();
    shaders.jacobi.setFloat('alpha', alpha);
    shaders.jacobi.setFloat('beta', beta);
    shaders.jacobi.setTexture('b', fields.temp.texture, 1);

    for (let i = 0; i < 20; i++) {
        shaders.jacobi.setTexture('x', swapFBO.front.texture, 0);
        drawQuad(swapFBO.back.buffer);
        swapFBO.swap();
    }
}

function copyFBO(dest, src) {
    shaders.copy.use();
    shaders.copy.setTexture('field', src.texture, 0);
    drawQuad(dest.buffer);
}

function drawQuad(buffer) {
    sim.quad.bind();
    gl.enableVertexAttribArray(0);
    gl.bindFramebuffer(gl.FRAMEBUFFER, buffer);
    gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);
}

function toHex(c) {
    let hex = c.toString(16);
    return hex.length == 1 ? "0" + hex : hex;
}

function rgbfToHex(vec) {
    return "#" + toHex((vec.x * 255).toFixed(0) | 0) + toHex((vec.y * 255).toFixed(0) | 0) + toHex((vec.z * 255).toFixed(0) | 0);
}

function hexToRgb(hex) {
    let result = /^#?([\da-f]{2})([\da-f]{2})([\da-f]{2})$/i.exec(hex);
    return result ? new Vec3(parseInt(result[1], 16) / 255, parseInt(result[2], 16) / 255, parseInt(result[3], 16) / 255) : null;
}

// Credit to mjackson: https://gist.github.com/mjackson/5311256
function hslToRgb(h, s, l){
    let r, g, b;

    if (s == 0) {
        r = g = b = l; // achromatic
    }
    else {
        function hue2rgb(p, q, t) {
            if(t < 0) t += 1;
            if(t > 1) t -= 1;
            if(t < 1/6) return p + (q - p) * 6 * t;
            if(t < 1/2) return q;
            if(t < 2/3) return p + (q - p) * (2/3 - t) * 6;
            return p;
        };

        let q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        let p = 2 * l - q;
        r = hue2rgb(p, q, h + 1/3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1/3);
    }

    return new Vec3(r, g, b);
};

var glsl = {
    vert: `
precision highp float;

attribute vec3 vertex;

uniform vec2 stride;

varying vec2 coord;
varying vec2 pxL;
varying vec2 pxR;
varying vec2 pxT;
varying vec2 pxB;

vec2 centerhalf(vec2 v)
{
    return v * 0.5 + 0.5;
}

void main()
{
    gl_Position = vec4(vertex.xy, 0.0, 1.0);

    coord = centerhalf(vertex.xy);
    pxL = coord - vec2(stride.x, 0);
    pxR = coord + vec2(stride.x, 0);
    pxB = coord - vec2(0, stride.y);
    pxT = coord + vec2(0, stride.y);
}`,
    adv: `
precision highp float;

uniform float delta_t;
uniform float dissipation;
uniform sampler2D velocity;
uniform sampler2D quantity;
uniform float gs;

varying vec2 coord;

void main()
{
    vec2 u1 = texture2D(velocity, coord).xy;
    
    vec2 pos0 = coord - delta_t * gs * u1;
    vec3 u0 = dissipation * texture2D(quantity, pos0).xyz;

    gl_FragColor = vec4(u0, 1.0);
}`,
    impulse: `
precision highp float;

uniform vec2 position;		// Cursor position
uniform vec3 force;			// The force
uniform float radius;		// Radius of gaussian splat
uniform sampler2D velocity;	// Velocity field

varying vec2 coord;

void main()
{
	vec2 diff = position - coord;
	float x = -dot(diff,diff) / radius;
	vec3 effect = force * exp(x);
	vec3 u0 = texture2D(velocity, coord).xyz;
    u0 += effect;

	gl_FragColor = vec4(u0, 1.0);
}`,
    jacobi: `precision highp float;

uniform float beta;
uniform float alpha;
uniform sampler2D x;
uniform sampler2D b;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

void main()
{
    vec3 xL = texture2D(x, pxL).xyz;
    vec3 xR = texture2D(x, pxR).xyz;
    vec3 xB = texture2D(x, pxB).xyz;
    vec3 xT = texture2D(x, pxT).xyz;
    vec3 bC = texture2D(b, coord).xyz;

    vec3 result = (xL + xR + xB + xT + (alpha * bC)) / beta;

    gl_FragColor = vec4(result, 1.0);
}`,
    vorticity: `precision highp float;

uniform sampler2D velocity;
uniform float gs;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

void main()
{
    vec2 R = texture2D(velocity, pxR).xy;
    vec2 L = texture2D(velocity, pxL).xy;
    vec2 B = texture2D(velocity, pxB).xy;
    vec2 T = texture2D(velocity, pxT).xy;
    
    float vorticity = ((R.y - L.y)/(2.0 * gs)) - ((T.x - B.x)/(2.0 * gs));

    gl_FragColor = vec4(vorticity, 0.0, 0.0, 1.0);
}`,
    addVorticity: `
precision highp float;

#define EPSILON 0.00024414

uniform sampler2D velocity;
uniform sampler2D vorticity;
uniform float scale;
uniform float delta_t;
uniform float gs;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

void main()
{
    float R = texture2D(vorticity, pxR).x;
    float L = texture2D(vorticity, pxL).x;
    float B = texture2D(vorticity, pxB).x;
    float T = texture2D(vorticity, pxT).x;
    float C = texture2D(vorticity, coord).x;

    vec2 force = vec2(abs(T) - abs(B), abs(R) - abs(L)) / (2.0 * gs);
    float mag_sq = max(EPSILON, dot(force,force));
    force *= inversesqrt(mag_sq);
    force *= scale * C * vec2(1.0, -1.0);

    vec2 u = texture2D(velocity, coord).xy;
    u += delta_t * force;

    gl_FragColor = vec4(u, 0.0, 1.0);
}`,
    gradient: `precision highp float;

uniform sampler2D field;
uniform float gs;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

void main()
{
    float R = texture2D(field, pxR).x;
    float L = texture2D(field, pxL).x;
    float B = texture2D(field, pxB).x;
    float T = texture2D(field, pxT).x;
    
    vec2 gradient = vec2(R-L, T-B)/(2.0 * gs);
    gl_FragColor = vec4(gradient, 0.0, 1.0);
}`,
    divergence: `precision highp float;

uniform sampler2D field;
uniform float gs;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

void main()
{
    vec2 R = texture2D(field, pxR).xy;
    vec2 L = texture2D(field, pxL).xy;
    vec2 B = texture2D(field, pxB).xy;
    vec2 T = texture2D(field, pxT).xy;
    
    float div = (R.x - L.x)/(2.0 * gs) + (T.y - B.y)/(2.0 * gs);

    gl_FragColor = vec4(div, 0.0, 0.0, 1.0);
}`,
    subtract: `precision highp float;

uniform sampler2D a;
uniform sampler2D b;

varying vec2 coord;

void main()
{
    vec2 txa = texture2D(a, coord).xy;
    vec2 txb = texture2D(b, coord).xy;
    vec2 diff = txa - txb;

    gl_FragColor = vec4(diff, 0.0, 1.0);
}`,
    scalar_vis: `
precision highp float;

uniform sampler2D field;
uniform vec4 bias;
uniform vec4 scale;

varying vec2 coord;

void main()
{
    vec4 vis = bias + scale * texture2D(field, coord).xxxx;
    gl_FragColor = vec4(vis.rgb, 1.0);
}`,
    vector_vis: `
precision highp float;

uniform sampler2D field;
uniform vec4 bias;
uniform vec4 scale;

varying vec2 coord;

void main()
{
    vec4 vis = bias + scale * texture2D(field, coord);
    gl_FragColor = vis;
}`,
    copy: `
precision highp float;

uniform sampler2D field;

varying vec2 coord;

void main()
{
    vec4 tx = texture2D(field, coord);
    gl_FragColor = tx;
}`,
    boundary: `
precision highp float;

uniform sampler2D field;
uniform vec2 rdv;
uniform vec2 offset;
uniform float scale;

varying vec2 coord;

void main()
{
	vec2 value = texture2D(field, coord + (offset * rdv)).xy;
	value = scale * value;
	gl_FragColor = vec4(value, 0.0, 1.0);
}`
};