import * as React from "react";
import * as ReactDOM from "react-dom";

declare let require:any
let mat4:any = require("gl-mat4")
let decompose:any = require("mat4-decompose")

let whitespace = /\s/
let blank = <div>&nbsp;</div>

class MatrixForm extends React.Component<any, any> {
  constructor(props:{}) {
    super(props);
    this.state = {
      mv: '', p:'', transpose:false
    };

    this.handleChangeMv = this.handleChangeMv.bind(this);
    this.handleChangeP = this.handleChangeP.bind(this);
  }

  handleChangeMv(event:any) { this.setState({mv: event.target.value}) }
  handleChangeP(event:any) { this.setState({p: event.target.value}) }
  setTranspose(v:boolean) { this.setState({transpose: v}) }

  render() {
    let decode = function(v:string, result:Float32Array) {
      v = v.trim()
      if (v) {
        let a = v.split(whitespace)
        for(let idx in a) {
          let coef = Number(a[idx])
          if (isNaN(coef))
            return "\"" + a[idx] + "\" is not a number"
          result[idx] = coef
        }
        if (a.length != 16)
          return "Expected 16 numbers, found " + a.length
        return null
      } else {
        return true
      }
    }
    let mv = mat4.create()
    let p = mat4.create()
    let mvFail = decode(this.state.mv, mv)
    let pFail = decode(this.state.p, p)

    if (!mvFail && this.state.transpose) mat4.transpose(mv,mv)
    if (!pFail && this.state.transpose) mat4.transpose(p,p)

    let mvp = mat4.create()
    if (this.state.transpose)
      mat4.multiply(mvp, mv, p)
    else
      mat4.multiply(mvp, p, mv)

    let renderFailure = function(failCode:string | boolean) {
      if (!failCode || failCode == true) return blank
      return <div className="failure">{failCode}</div>
    }

    let renderMatrix = function(m:Float32Array, transpose:boolean) {
      if (transpose) {
        let m2 = mat4.create()
        mat4.transpose(m2, m)
        m = m2
      }
      let rows:JSX.Element[] = Array()
      for(let y = 0; y < 4; y++) {
        let cols:JSX.Element[] = Array()
        for(let x = 0; x < 4; x++) {
          cols.push(<td key={x}>{m[y*4 + x].toPrecision(2)}</td>)
        }
        rows.push(<tr key={y}>{cols}</tr>)
      }
      return <table className="matrixPrint"><tbody>{rows}</tbody></table>
    }

    let renderVector = function(a:number[]) {
      let aStr = a.map(x => x.toPrecision(2)).join(", ")
      return <span className="vectorPrint">&lt;{aStr}&gt;</span>
    }

    let renderDecompose = function(m:Float32Array, suppress:boolean) {
      let renderTo6 = function(within:JSX.Element) {
        let trs:JSX.Element[] = Array()
        for(let c = 0; c < 6; c++) {
          trs.push(<tr key={c}><td>{c == 0 && within ? within : blank}</td></tr>)
        }
        return <table><tbody>{trs}</tbody></table>
      }
      if (suppress)
        return renderTo6(null)
      let translate = [0,0,0],
        scale = [0,0,0],
        skew = [0,0,0],
        perspective = [0,0,0,0],
        quaternion = [0,0,0,0]
      let success = decompose(m, translate, scale, skew, perspective, quaternion)
      if (!success)
        return renderTo6(<div className="failure">Failed to interpret</div>)
      return <table><tbody>
        <tr><td>Translate:</td><td>{renderVector(translate)}</td></tr>
        <tr><td>Scale:</td><td>{renderVector(scale)}</td></tr>
        <tr><td>Skew:</td><td>{renderVector(skew)}</td></tr>
        <tr><td>Perspective:</td><td>{renderVector(perspective)}</td></tr>
        <tr><td colSpan={2}>&nbsp;</td></tr>
        <tr><td>Quaternion:</td><td>{renderVector(quaternion)}</td></tr>
      </tbody></table>
    }

    let smallDivider = mvFail || pFail ? blank : <hr className="smallDivider" />

    return (
      <div className="MatrixForm">
        <p>Paste a matrix or two.</p>
        <table className="radioTable"><tbody><tr>
          <td><input id="a" type="radio"
            checked={!this.state.transpose} onChange={_ => this.setTranspose(false)} /></td>
          <td>Unchanged</td>
          <td><input id="b" type="radio"
            checked={this.state.transpose} onChange={_ => this.setTranspose(true)} /></td>
          <td>Transposed</td>
        </tr></tbody></table>
        <table className="mainTable">
          <tbody>
            <tr>
              <td>
                <label>Modelview:</label> <br/>
                <textarea value={this.state.mv} onChange={this.handleChangeMv} />
                {renderFailure(mvFail)}
              </td>
              <td>{renderDecompose(mv, Boolean(mvFail))}</td>
            </tr>
            <tr><td align="center"><strong>*</strong></td><td>{smallDivider}</td></tr>
            <tr>
              <td>
                <label>Projection:</label> <br/>
                <textarea value={this.state.p} onChange={this.handleChangeP} />
                {renderFailure(pFail)}
              </td>
              <td>{renderDecompose(p, Boolean(pFail))}</td>
            </tr>
            <tr><td align="center"><strong>=</strong></td><td>{smallDivider}</td></tr>
            <tr>
              <td align="center">
                {blank}
                {mvFail || pFail ? null : renderMatrix(mvp, this.state.transpose)}
                {blank}
              </td>
              <td>{renderDecompose(mvp, Boolean(mvFail || pFail))}</td>
            </tr>
          </tbody>
        </table>
        <p className="footer">
          Tool by <a href="https://data.runhello.com">andi mcc</a> based on <a href="https://github.com/mattdesl/mat4-decompose">mat4-decompose</a> by Matt DesLauriers. {}
          <a href="https://bitbucket.org/runhello/matrixcalc">Source here</a>.
        </p>
      </div>
    );
  }
}

ReactDOM.render(
  <MatrixForm />,
  document.getElementById("content")
);
