import React, {Component} from "react";

class ServerInfo extends Component {
    constructor(props) {
        super(props);

        this.state = {
            user: props.user,
            channels: []
        }
    }


    componentWillMount() {

    }

    componentDidMount() {

    }

    componentWillUnmount() {

    }

    render() {
        let userStyle = {
            backgroundImage: `url("${this.state.user.image}")`,
        };
        return (
          <div id="Server-Info" className="flexColumn">
              <div id="User-Profile" className="row flexFit">
                  <div className="col-3"><div className="userIcon-large shadow" style={userStyle}></div></div>
                  <div className="col-1"></div>
                  <div className="col-7 top7"><span className="userName">{this.state.user.profileName}</span></div>
              </div>
              <div id="Server-Channels">

              </div>
          </div>
        );
    }
}

export default ServerInfo;