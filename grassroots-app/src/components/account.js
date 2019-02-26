import React, { Component } from 'react';

class Account extends Component {
    state = {
        account_name: 'craig.tf',
        signed_in: 'true'
    }

    render() {
        return (
            <div>
                <h3>{this.state.account_name}</h3>
                <p>{this.state.signed_in}</p>
            </div>
        )
    }
}

export default Account;